############################################################################
# Bcg729Config.cmake
# Copyright (C) 2015  Belledonne Communications, Grenoble France
#
############################################################################
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
############################################################################
#
# Config file for the bcg729 package.
# It defines the following variables:
#
#  BCG729_FOUND - system has bcg729
#  BCG729_INCLUDE_DIRS - the bcg729 include directory
#  BCG729_LIBRARIES - The libraries needed to use bcg729

include("${CMAKE_CURRENT_LIST_DIR}/Bcg729Targets.cmake")

get_filename_component(BCG729_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(BCG729_INCLUDE_DIRS "${BCG729_CMAKE_DIR}/../../../include")
set(BCG729_LIBRARIES bcg729)
set(BCG729_FOUND 1)
