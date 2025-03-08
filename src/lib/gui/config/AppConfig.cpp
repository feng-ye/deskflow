/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "AppConfig.h"

#include "ConfigScopes.h"

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QVariant>

#include <functional>

using namespace deskflow::gui;

// this should be incremented each time the wizard is changed,
// which will force it to re-run for existing installations.
const int kWizardVersion = 8;

static const char *const kLogLevelNames[] = {"INFO", "DEBUG", "DEBUG1", "DEBUG2"};

#if defined(Q_OS_WIN)
const char AppConfig::m_LogDir[] = "log/";
#else
const char AppConfig::m_LogDir[] = "/var/log/";
#endif

// TODO: instead, use key value pair table, which would be less fragile.
const char *const AppConfig::m_SettingsName[] = {
    "screenName",
    "port",
    "interface",
    "logLevel2",
    "logToFile",
    "logFilename",
    "", // 6 wizardLastRun, obsolete
    "", // 7 statedBefore moved to deskflow settings
    "elevateMode",
    "elevateModeEnum",
    "", // 10 = edition, obsolete (using serial key instead)
    "", // 11 = kTlsEnabled (retain legacy string value) Moved to Settings
    "", // 12 AutoHide, moved to Settings
    "", // 13 = serialKey, obsolete
    "", // 14 last Version moved ot deskflow settings
    "", // 15 = lastExpiringWarningTime, obsolete
    "", // 16 = activationHasRun, obsolete
    "", // 17 = minimizeToTray, obsolete
    "", // 18 = ActivateEmail, obsolete
    "loadFromSystemScope",
    "groupServerChecked", // kServerGroupChecked
    "",                   // 21 = use external config moved to deskflow settings
    "",                   // 22 config file moved to dekflow settings
    "useInternalConfig",
    "groupClientChecked",
    "", // 25 server host name moved to deskflow settings
    "", // 26 cert path moved to deskflow settings
    "", // 27 key length Moved to Deskflow settings
    "", // 28 Prevent sleep moved to deskflow settings
    "", // 29 Language sync moved to deskflow settings
    "", // 30 = invertscrolldriection moved to deskflow settings
    "", // 31 = guid, obsolete
    "", // 32 = licenseRegistryUrl, obsolete
    "", // 33 = licenseNextCheck, obsolete
    "", // 34 = kInvertConnection, obsolete
    "", // 35 = clientHostMode, obsolete
    "", // 36 = serverClientMode, obsolete
    "enableService",
    "", // 38 Moved to deskflow settings
    "", // 39 window size moved to deskflow settings
    "", // 40 window position moved to deskflow settings
    "", // 41 = Show dev thanks, obsolete
    "", // 42 show Close Reminder moved to deskflow settings
    "", // 43 Moved to deskflow settings
    "", // 44, Moved to deskflow settings.
    "", // 45 Moved to deskflow settings
    "", // 46 require peer certs, Moved to deskflow settings
};

AppConfig::AppConfig(deskflow::gui::IConfigScopes &scopes, std::shared_ptr<Deps> deps)
    : m_Scopes(scopes),
      m_pDeps(deps),
      m_ScreenName(deps->hostname())
{
  determineScope();
  recall();
}

void AppConfig::recall()
{
  using enum AppConfig::Setting;

  qDebug("recalling app config");

  recallFromAllScopes();
  recallFromCurrentScope();
}

void AppConfig::recallFromAllScopes()
{
  using enum Setting;
  m_LoadFromSystemScope = findInAllScopes(kLoadSystemSettings, m_LoadFromSystemScope).toBool();
}

void AppConfig::recallFromCurrentScope()
{
  using enum Setting;

  recallScreenName();
  recallElevateMode();

  m_Port = getFromCurrentScope(kPort, m_Port).toInt();
  m_Interface = getFromCurrentScope(kInterface, m_Interface).toString();
  m_LogLevel = getFromCurrentScope(kLogLevel, m_LogLevel).toInt();
  m_LogToFile = getFromCurrentScope(kLogToFile, m_LogToFile).toBool();
  m_LogFilename = getFromCurrentScope(kLogFilename, m_LogFilename).toString();
  m_ServerGroupChecked = getFromCurrentScope(kServerGroupChecked, m_ServerGroupChecked).toBool();
  m_UseInternalConfig = getFromCurrentScope(kUseInternalConfig, m_UseInternalConfig).toBool();
  m_ClientGroupChecked = getFromCurrentScope(kClientGroupChecked, m_ClientGroupChecked).toBool();
  m_EnableService = getFromCurrentScope(kEnableService, m_EnableService).toBool();
}

void AppConfig::recallScreenName()
{
  using enum Setting;

  const auto &screenName = getFromCurrentScope(kScreenName, m_ScreenName).toString().trimmed();

  // for some reason, the screen name can be saved as an empty string
  // in the config file. this is probably a bug. if this happens, then default
  // back to the hostname.
  if (screenName.isEmpty()) {
    qWarning("screen name was empty in config, setting to hostname");
    m_ScreenName = m_pDeps->hostname();
  } else {
    m_ScreenName = screenName;
  }
}

void AppConfig::commit()
{
  using enum Setting;

  qDebug("committing app config");

  saveToAllScopes(kLoadSystemSettings, m_LoadFromSystemScope);
  saveToAllScopes(kClientGroupChecked, m_ClientGroupChecked);
  saveToAllScopes(kServerGroupChecked, m_ServerGroupChecked);

  if (isActiveScopeWritable()) {
    setInCurrentScope(kScreenName, m_ScreenName);
    setInCurrentScope(kPort, m_Port);
    setInCurrentScope(kInterface, m_Interface);
    setInCurrentScope(kLogLevel, m_LogLevel);
    setInCurrentScope(kLogToFile, m_LogToFile);
    setInCurrentScope(kLogFilename, m_LogFilename);
    setInCurrentScope(kElevateMode, static_cast<int>(m_ElevateMode));
    setInCurrentScope(kElevateModeLegacy, m_ElevateMode == ElevateMode::kAlways);
    setInCurrentScope(kUseInternalConfig, m_UseInternalConfig);
    setInCurrentScope(kEnableService, m_EnableService);
  }
}

void AppConfig::determineScope()
{

  qDebug("determining config scope");

  // first, try to determine if the system scope should be used according to the
  // user scope...
  if (m_Scopes.scopeContains(settingName(Setting::kLoadSystemSettings), ConfigScopes::Scope::User)) {
    auto loadFromSystemScope =
        m_Scopes
            .getFromScope(settingName(Setting::kLoadSystemSettings), m_LoadFromSystemScope, ConfigScopes::Scope::User)
            .toBool();
    if (loadFromSystemScope) {
      qDebug("user settings indicates system scope should be used");
    } else {
      qDebug("user settings indicates user scope should be used");
    }
    setLoadFromSystemScope(loadFromSystemScope);
  }

  // ...failing that, check the system scope instead to see if an arbitrary
  // required setting is present. if it is, then we can assume that the system
  // scope should be used.
  else if (m_Scopes.scopeContains(settingName(Setting::kScreenName), ConfigScopes::Scope::System)) {
    qDebug("system settings scope contains screen name, using system scope");
    setLoadFromSystemScope(true);
  }
}

void AppConfig::recallElevateMode()
{
  using enum Setting;

  if (!m_Scopes.scopeContains(settingName(kElevateMode))) {
    qDebug("elevate mode not set yet, skipping");
    return;
  }

  QVariant elevateMode = getFromCurrentScope(kElevateMode);
  if (!elevateMode.isValid()) {
    qDebug("elevate mode not valid, loading legacy setting");
    elevateMode = getFromCurrentScope(kElevateModeLegacy, QVariant(static_cast<int>(kDefaultElevateMode)));
  }

  m_ElevateMode = static_cast<ElevateMode>(elevateMode.toInt());
}

QString AppConfig::settingName(Setting name)
{
  auto index = static_cast<int>(name);
  return m_SettingsName[index];
}

template <typename T> void AppConfig::setInCurrentScope(Setting name, T value)
{
  m_Scopes.setInScope(settingName(name), value);
}

template <typename T> void AppConfig::saveToAllScopes(Setting name, T value)
{
  m_Scopes.setInScope(settingName(name), value, ConfigScopes::Scope::User);
  m_Scopes.setInScope(settingName(name), value, ConfigScopes::Scope::System);
}

QVariant AppConfig::getFromCurrentScope(Setting name, const QVariant &defaultValue) const
{
  return m_Scopes.getFromScope(settingName(name), defaultValue);
}

template <typename T>
std::optional<T> AppConfig::getFromCurrentScope(Setting name, std::function<T(const QVariant &)> toType) const
{
  if (m_Scopes.scopeContains(settingName(name))) {
    return toType(m_Scopes.getFromScope(settingName(name)));
  } else {
    return std::nullopt;
  }
}

template <typename T> void AppConfig::setInCurrentScope(Setting name, const std::optional<T> &value)
{
  if (value.has_value()) {
    m_Scopes.setInScope(settingName(name), value.value());
  }
}

QVariant AppConfig::findInAllScopes(Setting name, const QVariant &defaultValue) const
{
  using enum ConfigScopes::Scope;

  QVariant result(defaultValue);
  QString setting(settingName(name));

  if (m_Scopes.scopeContains(setting)) {
    result = m_Scopes.getFromScope(setting, defaultValue);
  } else if (m_Scopes.activeScope() == System) {
    if (m_Scopes.scopeContains(setting, User)) {
      result = m_Scopes.getFromScope(setting, defaultValue, User);
    }
  } else if (m_Scopes.scopeContains(setting, System)) {
    result = m_Scopes.getFromScope(setting, defaultValue, System);
  }

  return result;
}

void AppConfig::loadScope(ConfigScopes::Scope scope)
{
  using enum ConfigScopes::Scope;

  switch (scope) {
  case User:
    qDebug("loading user settings scope");
    break;

  case System:
    qDebug("loading system settings scope");
    break;

  default:
    qFatal("invalid scope");
  }

  if (m_Scopes.activeScope() == scope) {
    qDebug("already in required scope, skipping");
    return;
  }

  m_Scopes.setActiveScope(scope);

  qDebug("active scope file path: %s", qPrintable(m_Scopes.activeFilePath()));

  // only signal ready if there is at least one setting in the required scope.
  // this prevents the current settings from being set back to default.
  if (m_Scopes.scopeContains(settingName(Setting::kScreenName), m_Scopes.activeScope())) {
    m_Scopes.signalReady();
  } else {
    qDebug("no screen name in scope, skipping");
  }
}

void AppConfig::setLoadFromSystemScope(bool value)
{
  using enum ConfigScopes::Scope;

  if (value) {
    loadScope(System);
  } else {
    loadScope(User);
  }

  // set after loading scope since it may have been overridden.
  m_LoadFromSystemScope = value;
}

bool AppConfig::isActiveScopeWritable() const
{
  return m_Scopes.isActiveScopeWritable();
}

bool AppConfig::isActiveScopeSystem() const
{
  return m_Scopes.activeScope() == ConfigScopes::Scope::System;
}

QString AppConfig::logDir() const
{
  // by default log to home dir
  return QDir::home().absolutePath() + "/";
}

void AppConfig::persistLogDir() const
{
  QDir dir = logDir();

  // persist the log directory
  if (!dir.exists()) {
    dir.mkpath(dir.path());
  }
}

///////////////////////////////////////////////////////////////////////////////
// Begin getters
///////////////////////////////////////////////////////////////////////////////

IConfigScopes &AppConfig::scopes() const
{
  return m_Scopes;
}

const QString &AppConfig::screenName() const
{
  return m_ScreenName;
}

int AppConfig::port() const
{
  return m_Port;
}

const QString &AppConfig::networkInterface() const
{
  return m_Interface;
}

int AppConfig::logLevel() const
{
  return m_LogLevel;
}

bool AppConfig::logToFile() const
{
  return m_LogToFile;
}

const QString &AppConfig::logFilename() const
{
  return m_LogFilename;
}

QString AppConfig::logLevelText() const
{
  return kLogLevelNames[logLevel()];
}

ProcessMode AppConfig::processMode() const
{
  return m_EnableService ? ProcessMode::kService : ProcessMode::kDesktop;
}

ElevateMode AppConfig::elevateMode() const
{
  return m_ElevateMode;
}

bool AppConfig::enableService() const
{
  return m_EnableService;
}

bool AppConfig::serverGroupChecked() const
{
  return m_ServerGroupChecked;
}

bool AppConfig::useInternalConfig() const
{
  return m_UseInternalConfig;
}

bool AppConfig::clientGroupChecked() const
{
  return m_ClientGroupChecked;
}

///////////////////////////////////////////////////////////////////////////////
// End getters
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Begin setters
///////////////////////////////////////////////////////////////////////////////

void AppConfig::setServerGroupChecked(bool newValue)
{
  m_ServerGroupChecked = newValue;
}

void AppConfig::setUseInternalConfig(bool newValue)
{
  m_UseInternalConfig = newValue;
}

void AppConfig::setClientGroupChecked(bool newValue)
{
  m_ClientGroupChecked = newValue;
}

void AppConfig::setScreenName(const QString &s)
{
  m_ScreenName = s;
  Q_EMIT screenNameChanged();
}

void AppConfig::setPort(int i)
{
  m_Port = i;
}

void AppConfig::setNetworkInterface(const QString &s)
{
  m_Interface = s;
}

void AppConfig::setLogLevel(int i)
{
  const auto changed = (m_LogLevel != i);
  m_LogLevel = i;
  if (changed)
    Q_EMIT logLevelChanged();
}

void AppConfig::setLogToFile(bool b)
{
  m_LogToFile = b;
}

void AppConfig::setLogFilename(const QString &s)
{
  m_LogFilename = s;
}

void AppConfig::setElevateMode(ElevateMode em)
{
  m_ElevateMode = em;
}

void AppConfig::setEnableService(bool enabled)
{
  m_EnableService = enabled;
}

///////////////////////////////////////////////////////////////////////////////
// End setters
///////////////////////////////////////////////////////////////////////////////
