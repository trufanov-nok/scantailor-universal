IF(MSVC)
	# If we want reliable stack traces, we need /Oy-
        # We do it only for RelWithDebInfo
	SET(_common "/FS /wd4267")
	SET(CMAKE_C_FLAGS_RELEASE_INIT "/MD /O2 /Ob2 /D NDEBUG ${_common}")
	SET(CMAKE_C_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi /Ob0 /Od /RTC1 ${_common}")
	SET(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "/MD /Zi /O2 /Ob1 /D NDEBUG /Oy- ${_common}")
	SET(CMAKE_C_FLAGS_MINSIZEREL_INIT "/MD /O1 /Ob1 /D NDEBUG ${_common}")
ENDIF()
