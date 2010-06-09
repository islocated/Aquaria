CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(Aquaria)

INCLUDE(CheckCCompilerFlag)

IF(APPLE)
    SET(MACOSX TRUE)
ENDIF(APPLE)

OPTION(AQUARIA_BETAEXPIRE "This build should expire in 30 days." FALSE)
IF(AQUARIA_BETAEXPIRE)
    EXECUTE_PROCESS(
        COMMAND date +%s
        OUTPUT_VARIABLE BUILD_TIMESTAMP
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    ADD_DEFINITIONS(-DBETAEXPIRE=${BUILD_TIMESTAMP})
ENDIF(AQUARIA_BETAEXPIRE)

# No Steamworks SDK for Linux at the moment. Roll our own achievements.
ADD_DEFINITIONS(-DBBGE_BUILD_ACHIEVEMENTS_INTERNAL=1)

SET(BBGEDIR ${CMAKE_CURRENT_SOURCE_DIR}/BBGE)
SET(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/Aquaria)
SET(LUADIR ${CMAKE_CURRENT_SOURCE_DIR}/Aquaria/lua-5.1.4)
SET(LUASRCDIR ${LUADIR}/src)

# Find SDL or use the included library if applicable
find_package(SDL REQUIRED)
if (NOT SDL_FOUND)
    if(MACOSX)
        set(SDLDIR "${BBGEDIR}/SDL12")
        set(SDL_INCLUDE_DIR "${SDLDIR}/include")
        message(STATUS "Using internal copy of SDL")
        set(SDL_LIBRARY "${SDLDIR}/lib/macosx/libSDL-1.2.0.dylib ${SDLDIR}/lib/macosx/libSDLmain.a")
    else(MACOSX)
        message(ERROR "We don't have a prebuilt SDL for this platform.")
    endif(MACOSX)
endif (NOT SDL_FOUND)

# Find OpenAL or use the included library if applicable
find_package(OpenAL REQUIRED)
if (NOT OPENAL_FOUND)
    message(ERROR "We don't have a prebuilt OpenAL for this platform.")
endif (NOT OPENAL_FOUND)

SET(FTGLDIR ${BBGEDIR}/FTGL)
SET(FTGLSRCDIR ${FTGLDIR}/src)
SET(FREETYPE2DIR ${BBGEDIR}/freetype2)
SET(FREETYPE2SRCDIR ${FREETYPE2DIR}/src)
SET(LIBOGGDIR ${BBGEDIR}/libogg-1.1.3)
SET(LIBVORBISDIR ${BBGEDIR}/libvorbis-1.1.2)

INCLUDE_DIRECTORIES(${BBGEDIR})
INCLUDE_DIRECTORIES(${BBGEDIR}/GL)
INCLUDE_DIRECTORIES(${BBGEDIR}/glpng/zlib)
INCLUDE_DIRECTORIES(${SRCDIR})
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LUASRCDIR})
INCLUDE_DIRECTORIES(${FTGLDIR}/include)
INCLUDE_DIRECTORIES(${FREETYPE2DIR}/include)
INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LIBOGGDIR}/include)
INCLUDE_DIRECTORIES(${LIBVORBISDIR}/include)

# !!! FIXME: don't use fmod!
#INCLUDE_DIRECTORIES("${BBGEDIR}/fmodapi42604linux64/api/inc")

# Aquaria/BBGE specific defines...
ADD_DEFINITIONS(-DGL_GLEXT_LEGACY=1)
ADD_DEFINITIONS(-DHAVE_PUTENV=1)
ADD_DEFINITIONS(-DTIXML_USE_STL=1)
ADD_DEFINITIONS(-DBBGE_BUILD_SDL=1)
ADD_DEFINITIONS(-DBBGE_BUILD_FRAMEBUFFER=1)
#ADD_DEFINITIONS(-DBBGE_BUILD_SHADERS=1)
ADD_DEFINITIONS(-DBBGE_BUILD_OPENGL=1)
ADD_DEFINITIONS(-DBBGE_BUILD_OPENGL_DYNAMIC=1)
ADD_DEFINITIONS(-DBBGE_BUILD_WIDESCREEN=1)
ADD_DEFINITIONS(-DBBGE_BUILD_FMOD_OPENAL_BRIDGE=1)
ADD_DEFINITIONS(-DAQUARIA_NODRM=1)
ADD_DEFINITIONS(-DAQUARIA_FULL=1)

IF(UNIX)
    ADD_DEFINITIONS(-DBBGE_BUILD_UNIX=1)
ENDIF(UNIX)

IF(MACOSX)
    ADD_DEFINITIONS(-DBBGE_BUILD_MACOSX=1)
ENDIF(MACOSX)

IF(WIN32)
    ADD_DEFINITIONS(-DBBGE_BUILD_WINDOWS=1)
ENDIF(WIN32)


# Build Lua with Unix _setjmp/_longjmp support.
IF(UNIX)
    ADD_DEFINITIONS(-DLUA_USE_ULONGJMP=1)
ENDIF(UNIX)

IF(CMAKE_COMPILER_IS_GNUCC)
    ADD_DEFINITIONS(-pipe -fsigned-char -g)
    #ADD_DEFINITIONS(-Wall)

    # See if -fno-stack-protector is available to us.
    # It doesn't seem to work well, and it adds bulk to the binary.
    CHECK_C_COMPILER_FLAG("-fno-stack-protector" AQUARIA_GCC_HAS_STACKPROT)
    IF(AQUARIA_GCC_HAS_STACKPROT)
        ADD_DEFINITIONS(-fno-stack-protector)
    ENDIF(AQUARIA_GCC_HAS_STACKPROT)

    # !!! FIXME: probably not safe long-term.
    # CMake mailing list had this hack for getting rid of -rdynamic:
    #   http://public.kitware.com/pipermail/cmake/2006-July/010404.html
    IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
        SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
    ENDIF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

# Main game source code for Aquaria, minus engine and other middleware...
SET(AQUARIA_SRCS
    ${SRCDIR}/AnimationEditor.cpp
    ${SRCDIR}/AquariaComboBox.cpp
    ${SRCDIR}/AquariaMenuItem.cpp
    ${SRCDIR}/AquariaProgressBar.cpp
    ${SRCDIR}/AquariaSaveSlot.cpp
    ${SRCDIR}/AutoMap.cpp
    ${SRCDIR}/Avatar.cpp
    ${SRCDIR}/Beam.cpp
    ${SRCDIR}/BitBlotLogo.cpp
    ${SRCDIR}/BoxElement.cpp
    ${SRCDIR}/CollideEntity.cpp
    ${SRCDIR}/Continuity.cpp
    ${SRCDIR}/Credits.cpp
    ${SRCDIR}/CurrentRender.cpp
    ${SRCDIR}/Demo.cpp
    ${SRCDIR}/DSQ.cpp
    ${SRCDIR}/Element.cpp
    ${SRCDIR}/Emote.cpp
    ${SRCDIR}/Entity.cpp
    ${SRCDIR}/FlockEntity.cpp
    ${SRCDIR}/Game.cpp
    ${SRCDIR}/GameplayVariables.cpp
    ${SRCDIR}/GasCloud.cpp
    ${SRCDIR}/GridRender.cpp
    ${SRCDIR}/Hair.cpp
    ${SRCDIR}/Ingredient.cpp
    ${SRCDIR}/Intro.cpp
    ${SRCDIR}/Logo.cpp
    ${SRCDIR}/Main.cpp
    ${SRCDIR}/ManaBall.cpp
    ${SRCDIR}/MiniMapRender.cpp
    ${SRCDIR}/Mod.cpp
    ${SRCDIR}/ModSelector.cpp
    ${SRCDIR}/ParticleEditor.cpp
    ${SRCDIR}/Path.cpp
    ${SRCDIR}/PathFinding.cpp
    ${SRCDIR}/PathRender.cpp
    ${SRCDIR}/Protect.cpp
    ${SRCDIR}/RecipeMenuEntry.cpp
    ${SRCDIR}/SceneEditor.cpp
    ${SRCDIR}/SchoolFish.cpp
    ${SRCDIR}/ScriptedEntity.cpp
    ${SRCDIR}/ScriptInterface.cpp
    ${SRCDIR}/Segmented.cpp
    ${SRCDIR}/SFXLoops.cpp
    ${SRCDIR}/Shot.cpp
    ${SRCDIR}/Spore.cpp
    ${SRCDIR}/States.cpp
    ${SRCDIR}/StatsAndAchievements.cpp
    ${SRCDIR}/SteamRender.cpp
    ${SRCDIR}/Strand.cpp
    ${SRCDIR}/StringBank.cpp
    ${SRCDIR}/SubtitlePlayer.cpp
    ${SRCDIR}/ToolTip.cpp
    ${SRCDIR}/UserSettings.cpp
    ${SRCDIR}/WaterFont.cpp
    ${SRCDIR}/WaterSurfaceRender.cpp
    ${SRCDIR}/Web.cpp
    ${SRCDIR}/WorldMapRender.cpp
    ${SRCDIR}/WorldMapTiles.cpp
)

# Apparently not used at the moment. Listed here just for completeness.
SET(AQUARIA_SRCS_UNUSED
    ${SRCDIR}/BubbleRender.cpp
    ${SRCDIR}/FFTNotes.cpp
    ${SRCDIR}/StarRenderer.cpp
)

IF(MACOSX)
    SET(COCOA_SRCS "${BBGEDIR}/Cocoa.mm")
ENDIF(MACOSX)

# Bit Blot Game Engine sources...
SET(BBGE_SRCS
    ${BBGEDIR}/glfont2/glfont2.cpp
    ${BBGEDIR}/glpng/glpng.c
    ${BBGEDIR}/glpng/png/png.c
    ${BBGEDIR}/glpng/png/pngerror.c
    ${BBGEDIR}/glpng/png/pngget.c
    ${BBGEDIR}/glpng/png/pngmem.c
    ${BBGEDIR}/glpng/png/pngpread.c
    ${BBGEDIR}/glpng/png/pngread.c
    ${BBGEDIR}/glpng/png/pngrio.c
    ${BBGEDIR}/glpng/png/pngrtran.c
    ${BBGEDIR}/glpng/png/pngrutil.c
    ${BBGEDIR}/glpng/png/pngset.c
    ${BBGEDIR}/glpng/png/pngtrans.c
    ${BBGEDIR}/glpng/zlib/adler32.c
    ${BBGEDIR}/glpng/zlib/crc32.c
    ${BBGEDIR}/glpng/zlib/compress.c
    ${BBGEDIR}/glpng/zlib/deflate.c
    ${BBGEDIR}/glpng/zlib/inffast.c
    ${BBGEDIR}/glpng/zlib/inflate.c
    ${BBGEDIR}/glpng/zlib/inftrees.c
    ${BBGEDIR}/glpng/zlib/trees.c
    ${BBGEDIR}/glpng/zlib/uncompr.c
    ${BBGEDIR}/glpng/zlib/zutil.c
    ${BBGEDIR}/ActionInput.cpp
    ${BBGEDIR}/ActionMapper.cpp
    ${BBGEDIR}/ActionSet.cpp
    ${BBGEDIR}/AfterEffect.cpp
    ${BBGEDIR}/AnimatedSprite.cpp
    ${BBGEDIR}/Base.cpp
    ${BBGEDIR}/BitmapFont.cpp
    ${BBGEDIR}/Collision.cpp
    ${BBGEDIR}/Core.cpp
    ${BBGEDIR}/Cube.cpp
    ${BBGEDIR}/DarkLayer.cpp
    ${BBGEDIR}/Datafile.cpp
    ${BBGEDIR}/DebugFont.cpp
    ${BBGEDIR}/DFSprite.cpp
    ${BBGEDIR}/Effects.cpp
    ${BBGEDIR}/Emitter.cpp
    ${BBGEDIR}/Event.cpp
    ${BBGEDIR}/Flags.cpp
    ${BBGEDIR}/FrameBuffer.cpp
    ${BBGEDIR}/Gradient.cpp
    ${BBGEDIR}/Interpolator.cpp
    ${BBGEDIR}/Joystick.cpp
    ${BBGEDIR}/LensFlare.cpp
    ${BBGEDIR}/LightCone.cpp
    ${BBGEDIR}/Light.cpp
    ${BBGEDIR}/Math.cpp
    ${BBGEDIR}/PackRead.cpp
    ${BBGEDIR}/ParticleEffect.cpp
    ${BBGEDIR}/ParticleManager.cpp
    ${BBGEDIR}/Particles.cpp
    ${BBGEDIR}/Precacher.cpp
    ${BBGEDIR}/ProfRender.cpp
    ${BBGEDIR}/Quad.cpp
    ${BBGEDIR}/QuadTrail.cpp
    ${BBGEDIR}/RenderObject.cpp
    ${BBGEDIR}/RenderObjectLayer.cpp
    ${BBGEDIR}/RenderRect.cpp
    ${BBGEDIR}/Resource.cpp
    ${BBGEDIR}/RoundedRect.cpp
    ${BBGEDIR}/ScreenTransition.cpp
    ${BBGEDIR}/Shader.cpp
    ${BBGEDIR}/SkeletalSprite.cpp
    ${BBGEDIR}/Slider.cpp
    ${BBGEDIR}/SoundManager.cpp
    ${BBGEDIR}/SpawnParticleData.cpp
    ${BBGEDIR}/StateMachine.cpp
    ${BBGEDIR}/StateManager.cpp
    ${BBGEDIR}/Strings.cpp
    ${BBGEDIR}/Texture.cpp
    ${BBGEDIR}/tinyxml.cpp
    ${BBGEDIR}/tinyxmlerror.cpp
    ${BBGEDIR}/tinyxmlparser.cpp
    ${BBGEDIR}/TTFFont.cpp
    ${BBGEDIR}/Vector.cpp
    ${BBGEDIR}/FmodOpenALBridge.cpp
    ${COCOA_SRCS}
)

# Apparently not used at the moment. Listed here just for completeness.
SET(BBGE_SRCS_UNUSED
    ${BBGEDIR}/BloomEffect.cpp
    ${BBGEDIR}/CShim.cpp
    ${BBGEDIR}/Cutscene.cpp
    ${BBGEDIR}/FileVars.cpp
    ${BBGEDIR}/Model.cpp
    ${BBGEDIR}/OggStream.cpp
    ${BBGEDIR}/PointSprites.cpp
)

SET(FTGL_SRCS
    ${FTGLSRCDIR}/FTCharmap.cpp
    ${FTGLSRCDIR}/FTContour.cpp
    ${FTGLSRCDIR}/FTFace.cpp
    ${FTGLSRCDIR}/FTFont.cpp
    ${FTGLSRCDIR}/FTGLTextureFont.cpp
    ${FTGLSRCDIR}/FTGlyph.cpp
    ${FTGLSRCDIR}/FTGlyphContainer.cpp
    ${FTGLSRCDIR}/FTLibrary.cpp
    ${FTGLSRCDIR}/FTPoint.cpp
    ${FTGLSRCDIR}/FTSize.cpp
    ${FTGLSRCDIR}/FTTextureGlyph.cpp
)

SET(OGGVORBIS_SRCS
    ${LIBOGGDIR}/src/bitwise.c
    ${LIBOGGDIR}/src/framing.c
    ${LIBVORBISDIR}/lib/analysis.c
    ${LIBVORBISDIR}/lib/bitrate.c
    ${LIBVORBISDIR}/lib/block.c
    ${LIBVORBISDIR}/lib/codebook.c
    ${LIBVORBISDIR}/lib/envelope.c
    ${LIBVORBISDIR}/lib/floor0.c
    ${LIBVORBISDIR}/lib/floor1.c
    ${LIBVORBISDIR}/lib/info.c
    ${LIBVORBISDIR}/lib/lpc.c
    ${LIBVORBISDIR}/lib/lsp.c
    ${LIBVORBISDIR}/lib/mapping0.c
    ${LIBVORBISDIR}/lib/mdct.c
    ${LIBVORBISDIR}/lib/psy.c
    ${LIBVORBISDIR}/lib/registry.c
    ${LIBVORBISDIR}/lib/res0.c
    ${LIBVORBISDIR}/lib/sharedbook.c
    ${LIBVORBISDIR}/lib/smallft.c
    ${LIBVORBISDIR}/lib/synthesis.c
    ${LIBVORBISDIR}/lib/vorbisfile.c
    ${LIBVORBISDIR}/lib/window.c
)

SET(FREETYPE2_SRCS
    ${FREETYPE2SRCDIR}/base/ftsystem.c
    ${FREETYPE2SRCDIR}/base/ftdebug.c
    ${FREETYPE2SRCDIR}/base/ftinit.c
    ${FREETYPE2SRCDIR}/base/ftbase.c
    ${FREETYPE2SRCDIR}/base/ftbbox.c
    ${FREETYPE2SRCDIR}/base/ftbdf.c
    ${FREETYPE2SRCDIR}/base/ftbitmap.c
    ${FREETYPE2SRCDIR}/base/ftcid.c
    ${FREETYPE2SRCDIR}/base/ftfstype.c
    ${FREETYPE2SRCDIR}/base/ftgasp.c
    ${FREETYPE2SRCDIR}/base/ftglyph.c
    ${FREETYPE2SRCDIR}/base/ftgxval.c
    ${FREETYPE2SRCDIR}/base/ftlcdfil.c
    ${FREETYPE2SRCDIR}/base/ftmm.c
    ${FREETYPE2SRCDIR}/base/ftotval.c
    ${FREETYPE2SRCDIR}/base/ftpatent.c
    ${FREETYPE2SRCDIR}/base/ftpfr.c
    ${FREETYPE2SRCDIR}/base/ftstroke.c
    ${FREETYPE2SRCDIR}/base/ftsynth.c
    ${FREETYPE2SRCDIR}/base/fttype1.c
    ${FREETYPE2SRCDIR}/base/ftwinfnt.c
    ${FREETYPE2SRCDIR}/base/ftxf86.c
    ${FREETYPE2SRCDIR}/truetype/truetype.c
    ${FREETYPE2SRCDIR}/type1/type1.c
    ${FREETYPE2SRCDIR}/cff/cff.c
    ${FREETYPE2SRCDIR}/cid/type1cid.c
    ${FREETYPE2SRCDIR}/pfr/pfr.c
    ${FREETYPE2SRCDIR}/type42/type42.c
    ${FREETYPE2SRCDIR}/winfonts/winfnt.c
    ${FREETYPE2SRCDIR}/pcf/pcf.c
    ${FREETYPE2SRCDIR}/bdf/bdf.c
    ${FREETYPE2SRCDIR}/sfnt/sfnt.c
    ${FREETYPE2SRCDIR}/autofit/autofit.c
    ${FREETYPE2SRCDIR}/pshinter/pshinter.c
    ${FREETYPE2SRCDIR}/raster/raster.c
    ${FREETYPE2SRCDIR}/smooth/smooth.c
    ${FREETYPE2SRCDIR}/cache/ftcache.c
    ${FREETYPE2SRCDIR}/gzip/ftgzip.c
    ${FREETYPE2SRCDIR}/lzw/ftlzw.c
    ${FREETYPE2SRCDIR}/psaux/psaux.c
    ${FREETYPE2SRCDIR}/psnames/psmodule.c
)

SET_SOURCE_FILES_PROPERTIES(
    ${FREETYPE2_SRCS}
    PROPERTIES COMPILE_FLAGS "-DFT_CONFIG_OPTION_SYSTEM_ZLIB -DFT_CONFIG_CONFIG_H='\"${FREETYPE2DIR}/include/freetype/config/ftconfig.h\"' -DFT2_BUILD_LIBRARY -DFT_CONFIG_MODULES_H='\"${FREETYPE2DIR}/include/freetype/config/ftmodule.h\"' -I${FREETYPE2SRCDIR} -I${FREETYPE2DIR}/include/freetype/config -DHAVE_FCNTL_H"
)


SET(LUA_SRCS
    ${LUASRCDIR}/lapi.c
    ${LUASRCDIR}/ldebug.c
    ${LUASRCDIR}/ldo.c
    ${LUASRCDIR}/ldump.c
    ${LUASRCDIR}/lfunc.c
    ${LUASRCDIR}/lgc.c
    ${LUASRCDIR}/lmem.c
    ${LUASRCDIR}/lobject.c
    ${LUASRCDIR}/lopcodes.c
    ${LUASRCDIR}/lstate.c
    ${LUASRCDIR}/lstring.c
    ${LUASRCDIR}/ltable.c
    ${LUASRCDIR}/ltm.c
    ${LUASRCDIR}/lundump.c
    ${LUASRCDIR}/lvm.c
    ${LUASRCDIR}/lzio.c
    ${LUASRCDIR}/lauxlib.c
    ${LUASRCDIR}/lbaselib.c
    ${LUASRCDIR}/lstrlib.c
    ${LUASRCDIR}/ltablib.c
    ${LUASRCDIR}/lparser.c
    ${LUASRCDIR}/llex.c
    ${LUASRCDIR}/lcode.c
    ${LUASRCDIR}/lmathlib.c
)

ADD_EXECUTABLE(aquaria
    ${AQUARIA_SRCS}
    ${LUA_SRCS}
    ${BBGE_SRCS}
    ${FREETYPE2_SRCS}
    ${OGGVORBIS_SRCS}
    ${FTGL_SRCS}
    ${OPTIONAL_SRCS}
)

IF(MACOSX)
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "-framework Carbon")
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "-framework Cocoa")
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "-framework OpenGL")
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "-framework OpenAL")
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "${SDL_LIBRARY}")
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "${OPENAL_LIBRARY}")
    TARGET_LINK_LIBRARIES(aquaria ${OPTIONAL_LIBS})
ELSE(MACOSX)
    TARGET_LINK_LIBRARIES(aquaria ${SDL_LIBRARY} ${OPENAL_LIBRARY})
ENDIF(MACOSX)

# end of CMakeLists.txt ...

