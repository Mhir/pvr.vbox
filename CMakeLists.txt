project(pvr.vbox)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(TinyXML2 REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${TINYXML2_INCLUDE_DIRS}
                    ${PROJECT_SOURCE_DIR}/lib)

# Sources and headers
set(VBOX_SOURCES 
                src/client.h
                src/client.cpp)

set(VBOX_SOURCES_VBOX
                src/vbox/Channel.h
                src/vbox/ChannelStreamingStatus.h
                src/vbox/ChannelStreamingStatus.cpp
                src/vbox/Exceptions.h
                src/vbox/Recording.h
                src/vbox/Recording.cpp
                src/vbox/Settings.h
                src/vbox/SoftwareVersion.h
                src/vbox/SoftwareVersion.cpp
                src/vbox/StartupStateHandler.h
                src/vbox/VBox.h
                src/vbox/VBox.cpp)
                      
set(VBOX_SOURCES_VBOX_REQUEST 
                src/vbox/request/FileRequest.h
                src/vbox/request/IRequest.h
                src/vbox/request/Request.h
                src/vbox/request/Request.cpp)
                 
set(VBOX_SOURCES_VBOX_RESPONSE
                src/vbox/response/Content.h
                src/vbox/response/Content.cpp
                src/vbox/response/Factory.h
                src/vbox/response/Response.h
                src/vbox/response/Response.cpp)

set(VBOX_SOURCES_VBOX_TIMESHIFT 
                src/vbox/timeshift/Buffer.h
                src/vbox/timeshift/Buffer.cpp
                src/vbox/timeshift/DummyBuffer.h
                src/vbox/timeshift/FilesystemBuffer.h
                src/vbox/timeshift/FilesystemBuffer.cpp)

source_group("Source Files" FILES ${VBOX_SOURCES})
source_group("Source Files\\vbox" FILES ${VBOX_SOURCES_VBOX})
source_group("Source Files\\vbox\\request" FILES ${VBOX_SOURCES_VBOX_REQUEST})
source_group("Source Files\\vbox\\response" FILES ${VBOX_SOURCES_VBOX_RESPONSE})
source_group("Source Files\\vbox\\timeshift" FILES ${VBOX_SOURCES_VBOX_TIMESHIFT})
                 
# Resource files
set(VBOX_RESOURCES 
                pvr.vbox/addon.xml
                pvr.vbox/changelog.txt
                pvr.vbox/icon.png
                pvr.vbox/resources/settings.xml
                pvr.vbox/resources/language/English/strings.po)

source_group("Resource Files" FILES ${VBOX_RESOURCES})

# Append our custom file definitions
list(APPEND VBOX_SOURCES 
                ${VBOX_SOURCES_VBOX} 
                ${VBOX_SOURCES_VBOX_REQUEST}
                ${VBOX_SOURCES_VBOX_RESPONSE}
                ${VBOX_SOURCES_VBOX_TIMESHIFT}
                ${VBOX_RESOURCES})

# Add static libraries
add_subdirectory(lib/xmltv)

set(DEPLIBS ${kodiplatform_LIBRARIES} ${TINYXML2_LIBRARIES} libxmltv)
build_addon(pvr.vbox VBOX DEPLIBS)

include(CPack)
