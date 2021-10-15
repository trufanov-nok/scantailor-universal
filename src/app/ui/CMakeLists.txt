PROJECT(toplevel_ui_sources) # Don't rename - it's referenced elsewhere.

FILE(GLOB ui_files "*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

QT5_WRAP_UI(ui_sources ${ui_files})
ADD_CUSTOM_TARGET(toplevel_ui_sources SOURCES ${ui_sources})

TRANSLATION_SOURCES(scantailor-universal ${ui_files})
