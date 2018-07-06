cmake_minimum_required(VERSION 3.6)
# add_compile_options(-std=c++11)
project(gst_helloworld_cpp)

set(CMAKE_CXX_STANDARD 11)



find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB2 REQUIRED glib-2.0)
include_directories (${GLIB2_INCLUDE_DIRS})
link_directories (${GLIB2_LIBRARY_DIRS})

find_package (PkgConfig REQUIRED)
pkg_check_modules (GST REQUIRED gstreamer-1.0>=1.14)
include_directories (${GST_INCLUDE_DIRS})
link_directories (${GST_LIBRARY_DIRS})

include_directories($(/usr/local/Cellar/glib/2.56.1/include) /glib-2.0)


add_executable(gst_helloworld_cpp main.cpp)
# add_executable(gst_helloworld_cpp visvesh.cpp)
add_definitions(${GST-CFLAGS} ${GST_CFLAGS_OTHER})
target_link_libraries(gst_helloworld_cpp ${GST_LIBRARIES})

#target_link_libraries(gst_helloworld_cpp /usr/local/Cellar/gstreamer/1.14.1/lib/libgstbase-1.0.dylib)
#target_link_libraries(gst_helloworld_cpp /usr/local/Cellar/gst-plugins-base/1.14.1/lib/libgstvideo-1.0.dylib)
#target_link_libraries(gst_helloworld_cpp /usr/local/Cellar/gstreamer/1.14.1/lib/libgstnet-1.0.dylib)
#target_link_libraries(gst_helloworld_cpp /usr/local/Cellar/gstreamer/1.14.1/lib/libgstreamer-1.0.0.dylib)