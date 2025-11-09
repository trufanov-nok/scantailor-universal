set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "Alexander Traufanov <trufanovan@gmail.com>")

SET(CPACK_PACKAGE_NAME "scantailor-universal")

STRING(REGEX
        MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)"
        CPACK_PACKAGE_VERSION ${VERSION})

SET(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
SET(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
SET(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})

SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_EXECUTABLES "scantailor-deviant;${CMAKE_PROJECT_NAME} ${VERSION}")
SET(CPACK_CREATE_DESKTOP_LINKS "scantailor-deviant")

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "Alternative version of ScanTailor: interactive post-processing tool for scanned pages"
)
SET(CPACK_PACKAGE_DESCRIPTION
        "Scan Tailor is an interactive post-processing tool for scanned pages.\n"
        " It performs operations such as page splitting, deskewing, adding/removing\n"
        " borders, and others. You give it raw scans, and you get pages ready to be\n"
        " printed or assembled into a PDF or DJVU file. Scanning, optical character\n"
        " recognition, and assembling multi-page documents are out of scope of this\n"
        " project.")

SET(CPACK_STRIP_FILES YES)

SET(CPACK_SOURCE_IGNORE_FILES
        "/\\\\.svn/"
        "/\\\\.git/"
        "~$"
        "\\\\.pcs$"
        "TODO.txt"
        "CMakeLists.txt.user"
        "/doxygen/"
        "${CMAKE_BINARY_DIR}"
)

IF(WIN32)
    ADD_SUBDIRECTORY(windows)
ELSEIF(APPLE)
    ADD_SUBDIRECTORY(apple)
ELSE()
    ADD_SUBDIRECTORY(debian)
ENDIF()
