cmake_minimum_required(VERSION 3.6)

project(updater)

file(GLOB_RECURSE CURRENT_HEADERS src/*.h include/*.h src/*.hpp include/*.hpp)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src srcDir)

add_executable(${PROJECT_NAME} ${srcDir} ${CURRENT_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
  if(MSVC)
    # add_compile_options("/utf-8")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}) 
    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/out/win32/ia32/Debug")
    source_group("Header Files" FILES ${CURRENT_HEADERS})
    add_definitions("-D_CRT_SECURE_NO_WARNINGS -DUNICODE -D_UNICODE")
	target_link_libraries(${PROJECT_NAME} "comctl32")
    set(
      CompilerFlags
      # CMAKE_CXX_FLAGS
      # CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_RELEASE
      # CMAKE_C_FLAGS
      # CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_RELEASE
    )
    foreach(CompilerFlag ${CompilerFlags})
      string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()

    set(CMAKE_C_FLAGS "/utf-8 ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "/utf-8 ${CMAKE_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:WINDOWS /manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\" ${CMAKE_EXE_LINKER_FLAGS}")
  endif(MSVC)
else()
  set(CMAKE_C_FLAGS_RELEASE "-static ${CMAKE_C_FLAGS_RELEASE}")
  set(CMAKE_CXX_FLAGS_RELEASE "-static ${CMAKE_CXX_FLAGS_RELEASE}")
endif(WIN32)
