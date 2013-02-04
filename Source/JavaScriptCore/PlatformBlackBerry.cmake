LIST(INSERT JavaScriptCore_INCLUDE_DIRECTORIES 0
    "${BLACKBERRY_THIRD_PARTY_DIR}/icu"
)

LIST(REMOVE_ITEM JavaScriptCore_SOURCES
    runtime/GCActivityCallback.cpp
)

LIST(APPEND JavaScriptCore_SOURCES
    runtime/GCActivityCallbackBlackBerry.cpp
    runtime/MemoryStatistics.cpp
    DisassemblerARM.cpp
)

#FIXME: We will build libjavascriptcore.so as an additional library
#       See PR 155134 for more info
IF (${JavaScriptCore_LIBRARY_TYPE} STREQUAL "STATIC")
    ADD_LIBRARY(${JavaScriptCore_LIBRARY_NAME}_SHARED SHARED ${JavaScriptCore_HEADERS} ${JavaScriptCore_SOURCES})
    TARGET_LINK_LIBRARIES(${JavaScriptCore_LIBRARY_NAME}_SHARED ${JavaScriptCore_LIBRARIES})

    IF (JavaScriptCore_LINK_FLAGS)
        ADD_TARGET_PROPERTIES(${JavaScriptCore_LIBRARY_NAME}_SHARED LINK_FLAGS "${JavaScriptCore_LINK_FLAGS}")
    ENDIF ()
    SET_TARGET_PROPERTIES(${JavaScriptCore_LIBRARY_NAME}_SHARED PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
    SET_TARGET_PROPERTIES(${JavaScriptCore_LIBRARY_NAME}_SHARED PROPERTIES OUTPUT_NAME ${JavaScriptCore_LIBRARY_NAME})
    INSTALL(TARGETS ${JavaScriptCore_LIBRARY_NAME}_SHARED DESTINATION lib)
ENDIF ()
SET(JavaScriptCore_API_HEADERS
    "${JAVASCRIPTCORE_DIR}/API/JSBase.h"
    "${JAVASCRIPTCORE_DIR}/API/JSContextRef.h"
    "${JAVASCRIPTCORE_DIR}/API/JSObjectRef.h"
    "${JAVASCRIPTCORE_DIR}/API/JSStringRef.h"
    "${JAVASCRIPTCORE_DIR}/API/JSValueRef.h"
    "${JAVASCRIPTCORE_DIR}/API/JavaScript.h"
    "${JAVASCRIPTCORE_DIR}/API/WebKitAvailability.h"
)
INSTALL(FILES ${JavaScriptCore_API_HEADERS} DESTINATION ../../usr/include/browser/webkit/javascriptcore)

#FIXME: We need to fix the relative path here
INSTALL(FILES "../WTF/wtf/Forward.h" DESTINATION ../../usr/include/browser/webkit/wtf)
