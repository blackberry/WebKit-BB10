if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
endif ()

LIST(INSERT WebCore_INCLUDE_DIRECTORIES 0
    "${BLACKBERRY_THIRD_PARTY_DIR}" # For <unicode.h>, which is included from <sys/keycodes.h>.
    "${BLACKBERRY_THIRD_PARTY_DIR}/icu"
)

LIST(REMOVE_ITEM WebCore_SOURCES
    page/scrolling/ScrollingCoordinatorNone.cpp
    html/shadow/MediaControlsApple.cpp
)

LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/blackberry/CookieDatabaseBackingStore"
    "${WEBCORE_DIR}/platform/graphics/gpu"
    "${WEBCORE_DIR}/platform/network/blackberry"
    "${WEBCORE_DIR}/platform/network/blackberry/rss"
    "${WEBCORE_DIR}/platform/graphics/opentype"
)

# BlackBerry graphics
LIST(APPEND WebCore_SOURCES
    platform/graphics/FontPlatformData.cpp
    platform/graphics/blackberry/FontBlackBerry.cpp
    platform/graphics/blackberry/FontCustomPlatformDataBlackBerry.cpp
    platform/graphics/blackberry/FontCacheBlackBerry.cpp
    platform/graphics/blackberry/FontPlatformDataBlackBerry.cpp
    platform/graphics/blackberry/GlyphPageTreeNodeBlackBerry.cpp
    platform/graphics/blackberry/GradientBlackBerry.cpp
    platform/graphics/blackberry/GraphicsContextBlackBerry.cpp
    platform/graphics/blackberry/ImageBlackBerry.cpp
    platform/graphics/blackberry/ImageBufferBlackBerry.cpp
    platform/graphics/blackberry/PathBlackBerry.cpp
    platform/graphics/blackberry/PatternBlackBerry.cpp
    platform/graphics/blackberry/PlatformSupport.cpp
    platform/graphics/blackberry/SimpleFontDataBlackBerry.cpp
    platform/graphics/harfbuzz/HarfBuzzShaperBase.cpp
    platform/graphics/harfbuzz/ng/HarfBuzzNGFace.cpp
    platform/graphics/harfbuzz/ng/HarfBuzzNGFaceIType.cpp
    platform/graphics/harfbuzz/ng/HarfBuzzShaper.cpp
    platform/image-encoders/JPEGImageEncoder.cpp
    platform/image-encoders/PNGImageEncoder.cpp
)

# Other sources
LIST(APPEND WebCore_SOURCES
    html/shadow/MediaControlsBlackBerry.cpp
    platform/blackberry/CookieDatabaseBackingStore/CookieDatabaseBackingStore.cpp
    platform/blackberry/AuthenticationChallengeManager.cpp
    platform/blackberry/CookieManager.cpp
    platform/blackberry/CookieMap.cpp
    platform/blackberry/CookieParser.cpp
    platform/blackberry/FileSystemBlackBerry.cpp
    platform/blackberry/ParsedCookie.cpp
    platform/graphics/ImageSource.cpp
    platform/graphics/WOFFFileFormat.cpp
    platform/graphics/opentype/OpenTypeSanitizer.cpp
    platform/image-decoders/ImageDecoder.cpp
    platform/image-decoders/bmp/BMPImageDecoder.cpp
    platform/image-decoders/bmp/BMPImageReader.cpp
    platform/image-decoders/gif/GIFImageDecoder.cpp
    platform/image-decoders/gif/GIFImageReader.cpp
    platform/image-decoders/ico/ICOImageDecoder.cpp
    platform/image-decoders/jpeg/JPEGImageDecoder.cpp
    platform/image-decoders/png/PNGImageDecoder.cpp
    platform/image-decoders/webp/WEBPImageDecoder.cpp
    platform/posix/FileSystemPOSIX.cpp
    platform/posix/SharedBufferPOSIX.cpp
    platform/text/TextBreakIteratorICU.cpp
    platform/text/TextCodecICU.cpp
    platform/text/TextEncodingDetectorICU.cpp
    platform/text/blackberry/TextBreakIteratorInternalICUBlackBerry.cpp
)

# Networking sources
LIST(APPEND WebCore_SOURCES
    platform/network/MIMESniffing.cpp
    platform/network/blackberry/AutofillBackingStore.cpp
    platform/network/blackberry/DNSBlackBerry.cpp
    platform/network/blackberry/DeferredData.cpp
    platform/network/blackberry/NetworkJob.cpp
    platform/network/blackberry/NetworkManager.cpp
    platform/network/blackberry/NetworkStateNotifierBlackBerry.cpp
    platform/network/blackberry/ProxyServerBlackBerry.cpp
    platform/network/blackberry/ResourceErrorBlackBerry.cpp
    platform/network/blackberry/ResourceHandleBlackBerry.cpp
    platform/network/blackberry/ResourceRequestBlackBerry.cpp
    platform/network/blackberry/ResourceResponseBlackBerry.cpp
    platform/network/blackberry/SocketStreamHandleBlackBerry.cpp
    platform/network/blackberry/rss/RSSAtomParser.cpp
    platform/network/blackberry/rss/RSS10Parser.cpp
    platform/network/blackberry/rss/RSS20Parser.cpp
    platform/network/blackberry/rss/RSSFilterStream.cpp
    platform/network/blackberry/rss/RSSGenerator.cpp
    platform/network/blackberry/rss/RSSParserBase.cpp
)

LIST(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/mediaControlsBlackBerry.css
    ${WEBCORE_DIR}/css/mediaControlsBlackBerryFullscreen.css
    ${WEBCORE_DIR}/css/themeBlackBerry.css
)

LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/bridge/blackberry"
    "${WEBCORE_DIR}/history/blackberry"
    "${WEBCORE_DIR}/page/blackberry"
    "${WEBCORE_DIR}/platform/blackberry"
    "${WEBCORE_DIR}/platform/graphics/blackberry"
    "${WEBCORE_DIR}/platform/image-encoders"
    "${WEBCORE_DIR}/platform/text/blackberry"
    "${WEBKIT_DIR}/blackberry/Api"
    "${WEBKIT_DIR}/blackberry/WebCoreSupport"
    "${WEBKIT_DIR}/blackberry/WebKitSupport"
)

# BlackBerry sources
LIST(APPEND WebCore_SOURCES
    editing/blackberry/EditorBlackBerry.cpp
    editing/blackberry/SmartReplaceBlackBerry.cpp
    html/shadow/MediaControlsBlackBerry.cpp
    loader/blackberry/CookieJarBlackBerry.cpp
    page/blackberry/DragControllerBlackBerry.cpp
    page/blackberry/EventHandlerBlackBerry.cpp
    page/blackberry/SettingsBlackBerry.cpp
    page/scrolling/blackberry/ScrollingCoordinatorBlackBerry.cpp
    platform/blackberry/ClipboardBlackBerry.cpp
    platform/blackberry/ContextMenuBlackBerry.cpp
    platform/blackberry/ContextMenuItemBlackBerry.cpp
    platform/blackberry/CursorBlackBerry.cpp
    platform/blackberry/DragDataBlackBerry.cpp
    platform/blackberry/DragImageBlackBerry.cpp
    platform/blackberry/EventLoopBlackBerry.cpp
    platform/blackberry/KURLBlackBerry.cpp
    platform/blackberry/LocalizedStringsBlackBerry.cpp
    platform/blackberry/LoggingBlackBerry.cpp
    platform/blackberry/MIMETypeRegistryBlackBerry.cpp
    platform/blackberry/PasteboardBlackBerry.cpp
    platform/blackberry/PlatformKeyboardEventBlackBerry.cpp
    platform/blackberry/PlatformMouseEventBlackBerry.cpp
    platform/blackberry/PlatformScreenBlackBerry.cpp
    platform/blackberry/PlatformTouchEventBlackBerry.cpp
    platform/blackberry/PlatformTouchPointBlackBerry.cpp
    platform/blackberry/PopupMenuBlackBerry.cpp
    platform/blackberry/RenderThemeBlackBerry.cpp
    platform/blackberry/RunLoopBlackBerry.cpp
    platform/blackberry/SSLKeyGeneratorBlackBerry.cpp
    platform/blackberry/ScrollbarThemeBlackBerry.cpp
    platform/blackberry/SearchPopupMenuBlackBerry.cpp
    platform/blackberry/SharedTimerBlackBerry.cpp
    platform/blackberry/SoundBlackBerry.cpp
    platform/blackberry/SystemTimeBlackBerry.cpp
    platform/blackberry/TemporaryLinkStubs.cpp
    platform/blackberry/WidgetBlackBerry.cpp
    platform/graphics/blackberry/FloatPointBlackBerry.cpp
    platform/graphics/blackberry/FloatRectBlackBerry.cpp
    platform/graphics/blackberry/FloatSizeBlackBerry.cpp
    platform/graphics/blackberry/IconBlackBerry.cpp
    platform/graphics/blackberry/ImageBlackBerry.cpp
    platform/graphics/blackberry/IntPointBlackBerry.cpp
    platform/graphics/blackberry/IntRectBlackBerry.cpp
    platform/graphics/blackberry/IntSizeBlackBerry.cpp
    platform/graphics/blackberry/MediaPlayerPrivateBlackBerry.cpp
    platform/text/blackberry/StringBlackBerry.cpp
)

# Credential Persistence sources
LIST(APPEND WebCore_SOURCES
    platform/network/blackberry/CredentialBackingStore.cpp
    platform/network/blackberry/CredentialStorageBlackBerry.cpp
)

# File System support
IF (ENABLE_FILE_SYSTEM)
    LIST(APPEND WebCore_SOURCES
        platform/blackberry/AsyncFileSystemBlackBerry.cpp
        platform/blackberry/DOMFileSystemBlackBerry.cpp
        platform/blackberry/AsyncFileWriterBlackBerry.cpp
        platform/blackberry/LocalFileSystemBlackBerry.cpp
        platform/blackberry/PlatformAsyncFileSystemCallbacks.cpp
        platform/blackberry/PlatformBlob.cpp
        platform/blackberry/PlatformFileWriterClient.cpp
        platform/blackberry/WorkerAsyncFileSystemBlackBerry.cpp
        platform/blackberry/WorkerAsyncFileWriterBlackBerry.cpp
        platform/blackberry/WorkerPlatformAsyncFileSystemCallbacks.cpp
        platform/blackberry/WorkerPlatformFileWriterClient.cpp
    )
ENDIF ()

IF (ENABLE_SMOOTH_SCROLLING)
    LIST(APPEND WebCore_SOURCES
        platform/blackberry/ScrollAnimatorBlackBerry.cpp
    )
ENDIF ()

if (ENABLE_REQUEST_ANIMATION_FRAME)
    LIST(APPEND WebCore_SOURCES
        platform/graphics/blackberry/DisplayRefreshMonitorBlackBerry.cpp
        platform/graphics/DisplayRefreshMonitor.cpp
    )
ENDIF ()

if (ENABLE_WEBGL)
    ADD_DEFINITIONS (-DWTF_USE_OPENGL_ES_2=1)
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES
        "${THIRDPARTY_DIR}"
        "${WEBCORE_DIR}/platform/graphics/opengl"
        "${WTF_DIR}/wtf"
        "${WTF_DIR}/wtf/text"
    )
    LIST(APPEND WebCore_SOURCES
        platform/graphics/blackberry/DrawingBufferBlackBerry.cpp
        platform/graphics/blackberry/GraphicsContext3DBlackBerry.cpp
        platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
        platform/graphics/opengl/GraphicsContext3DOpenGLES.cpp
        platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
        platform/graphics/opengl/Extensions3DOpenGLES.cpp
        platform/graphics/gpu/SharedGraphicsContext3D.cpp
    )
ENDIF ()

if (ENABLE_MEDIA_STREAM)
    LIST(APPEND WebCore_SOURCES
        platform/mediastream/blackberry/MediaStreamCenterBlackBerry.cpp
    )
ENDIF ()

IF (ENABLE_NETSCAPE_PLUGIN_API)
    LIST(APPEND WebCore_SOURCES
        plugins/PluginDatabase.cpp
        plugins/PluginPackage.cpp
        plugins/PluginView.cpp
        plugins/blackberry/NPCallbacksBlackBerry.cpp
        plugins/blackberry/PluginDataBlackBerry.cpp
        plugins/blackberry/PluginPackageBlackBerry.cpp
        plugins/blackberry/PluginViewBlackBerry.cpp
        plugins/blackberry/PluginViewPrivateBlackBerry.cpp
    )
ELSE ()
    LIST(APPEND WebCore_SOURCES
        plugins/PluginDataNone.cpp
        plugins/PluginDatabase.cpp
        plugins/PluginPackage.cpp
        plugins/PluginPackageNone.cpp
        plugins/PluginView.cpp
        plugins/PluginViewNone.cpp
    )
ENDIF ()

# To speed up linking when working on accel comp, you can move this whole chunk
# to Source/WebKit/blackberry/CMakeListsBlackBerry.txt.
# Append to WebKit_SOURCES instead of WebCore_SOURCES.
IF (WTF_USE_ACCELERATED_COMPOSITING)
    LIST(APPEND WebCore_SOURCES
        ${WEBCORE_DIR}/platform/graphics/blackberry/CanvasLayerWebKitThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/EGLImageLayerWebKitThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/EGLImageLayerCompositingThreadClient.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/GraphicsLayerBlackBerry.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerAnimation.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerCompositingThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerFilterRenderer.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerRenderer.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerRendererSurface.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerTexture.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerTile.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerTiler.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerWebKitThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/PluginLayerWebKitThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/TextureCacheCompositingThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/VideoLayerWebKitThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/WebGLLayerWebKitThread.cpp
    )
ENDIF ()

SET(WebCore_LINK_FLAGS "${WebCore_LINK_FLAGS} -Wl,--no-keep-memory")

SET(ENV{WEBKITDIR} ${CMAKE_SOURCE_DIR})
SET(ENV{PLATFORMNAME} ${CMAKE_SYSTEM_NAME})
EXECUTE_PROCESS(
    COMMAND hostname
    OUTPUT_VARIABLE host
)
STRING(REPLACE "\n" "" host1 "${host}")
SET(ENV{COMPUTERNAME} ${host1})

IF ($ENV{PUBLIC_BUILD})
    ADD_DEFINITIONS(-DPUBLIC_BUILD=$ENV{PUBLIC_BUILD})
ENDIF ()

IF (ENABLE_WEBDOM)
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES "${WEBCORE_DIR}/bindings/cpp")

    LIST(APPEND WebCore_SOURCES
        bindings/cpp/WebDOMCString.cpp
        bindings/cpp/WebDOMEventTarget.cpp
        bindings/cpp/WebDOMString.cpp
        bindings/cpp/WebExceptionHandler.cpp
    )

    LIST(APPEND WEBDOM_IDL_HEADERS
        bindings/cpp/WebDOMCString.h
        bindings/cpp/WebDOMEventTarget.h
        bindings/cpp/WebDOMObject.h
        bindings/cpp/WebDOMString.h
    )

    INSTALL(FILES ${WEBDOM_IDL_HEADERS} DESTINATION usr/include/browser/webkit/dom)


SET(WebCore_CPP_IDL_FILES
    dom/EventListener.idl
    "${WebCore_CPP_IDL_FILES}"
)

FOREACH (_idl ${WebCore_CPP_IDL_FILES})
    SET(IDL_FILES_LIST "${IDL_FILES_LIST}${WEBCORE_DIR}/${_idl}\n")
ENDFOREACH ()

ENDIF ()

FILE(WRITE ${IDL_FILES_TMP} ${IDL_FILES_LIST})

ADD_CUSTOM_COMMAND(
    OUTPUT ${SUPPLEMENTAL_DEPENDENCY_FILE}
    DEPENDS ${WEBCORE_DIR}/bindings/scripts/preprocess-idls.pl ${SCRIPTS_RESOLVE_SUPPLEMENTAL} ${WebCore_CPP_IDL_FILES} ${IDL_ATTRIBUTES_FILE}
    COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${WEBCORE_DIR}/bindings/scripts/preprocess-idls.pl --defines "${FEATURE_DEFINES_JAVASCRIPT}" --idlFilesList ${IDL_FILES_TMP} --preprocessor "${CODE_GENERATOR_PREPROCESSOR}" --supplementalDependencyFile ${SUPPLEMENTAL_DEPENDENCY_FILE} --idlAttributesFile ${IDL_ATTRIBUTES_FILE}
    VERBATIM)

GENERATE_BINDINGS(WebCore_SOURCES
    "${WebCore_CPP_IDL_FILES}"
    "${WEBCORE_DIR}"
    "${IDL_INCLUDES}"
    "${FEATURE_DEFINES_WEBCORE}"
    ${DERIVED_SOURCES_WEBCORE_DIR} WebDOM CPP
    ${SUPPLEMENTAL_DEPENDENCY_FILE})

FOREACH (_file ${WebCore_CPP_IDL_FILES})
    GET_FILENAME_COMPONENT (_name ${_file} NAME_WE)
    ADD_CUSTOM_COMMAND(
        OUTPUT  ${DERIVED_SOURCES_WEBCORE_DIR}/WebDOM${_name}.cpp ${DERIVED_SOURCES_WEBCORE_DIR}/WebDOM${_name}.h
        MAIN_DEPENDENCY ${_file}
        DEPENDS ${WEBCORE_DIR}/bindings/scripts/generate-bindings.pl ${SCRIPTS_BINDINGS} ${WEBCORE_DIR}/bindings/scripts/CodeGeneratorCPP.pm ${SUPPLEMENTAL_DEPENDENCY_FILE} ${_file}
        COMMAND ${PERL_EXECUTABLE} -I${WEBCORE_DIR}/bindings/scripts ${WEBCORE_DIR}/bindings/scripts/generate-bindings.pl --defines "${FEATURE_DEFINES_WEBCORE}" --generator CPP ${IDL_INCLUDES} --outputDir "${DERIVED_SOURCES_WEBCORE_DIR}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}" --supplementalDependencyFile ${SUPPLEMENTAL_DEPENDENCY_FILE} ${WEBCORE_DIR}/${_file}
        VERBATIM)
    LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/WebDOM${_name}.cpp)
ENDFOREACH ()

# Generate contents for PopupPicker.cpp
SET(WebCore_POPUP_CSS_AND_JS
    ${WEBCORE_DIR}/Resources/blackberry/popupControlBlackBerry.css
    ${WEBCORE_DIR}/Resources/blackberry/selectControlBlackBerry.css
    ${WEBCORE_DIR}/Resources/blackberry/selectControlBlackBerry.js
    ${WEBCORE_DIR}/Resources/blackberry/timeControlBlackBerry.js
    ${WEBCORE_DIR}/Resources/blackberry/timeControlBlackBerry.css
    ${WEBCORE_DIR}/Resources/blackberry/colorControlBlackBerry.js
    ${WEBCORE_DIR}/Resources/blackberry/colorControlBlackBerry.css
)

ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/PopupPicker.h ${DERIVED_SOURCES_WEBCORE_DIR}/PopupPicker.cpp
    MAIN_DEPENDENCY ${WEBCORE_DIR}/make-file-arrays.py
    DEPENDS ${WebCore_POPUP_CSS_AND_JS}
    COMMAND ${PYTHON_EXECUTABLE} ${WEBCORE_DIR}/make-file-arrays.py --out-h=${DERIVED_SOURCES_WEBCORE_DIR}/PopupPicker.h --out-cpp=${DERIVED_SOURCES_WEBCORE_DIR}/PopupPicker.cpp ${WebCore_POPUP_CSS_AND_JS}
)
LIST(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/PopupPicker.cpp)
