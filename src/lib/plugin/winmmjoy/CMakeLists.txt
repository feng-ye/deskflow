# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Synergy Si Ltd.
# Copyright (C) 2012 Nick Bolton
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

file(GLOB headers "*.h")
file(GLOB sources "*.cpp")

if (SYNERGY_ADD_HEADERS)
	list(APPEND sources ${headers})
endif()

add_library(winmmjoy SHARED ${sources})
set_target_properties(winmmjoy PROPERTIES PREFIX "")
target_link_libraries(winmmjoy Winmm)

add_custom_command(
	TARGET winmmjoy
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory
		"${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/plugins/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${LIBRARY_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/winmmjoy.dll"
		"${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/plugins/"
)
