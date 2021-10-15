PROJECT(core_ui_sources) # Don't rename - it's referenced elsewhere.

FILE(GLOB core_ui_files "*.ui")
SOURCE_GROUP("UI Files" FILES ${core_ui_files})

QT5_WRAP_UI(c_ui_sources ${core_ui_files})
ADD_CUSTOM_TARGET(core_ui_sources SOURCES ${c_ui_sources})

TRANSLATION_SOURCES(scantailor-universal ${core_ui_files})
