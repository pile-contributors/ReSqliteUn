
# enable/disable cmake debug messages related to this pile
set (RESQLITEUN_DEBUG_MSG OFF)

# make sure support code is present; no harm
# in including it twice; the user, however, should have used
# pileInclude() from pile_support.cmake module.
include(pile_support)


# find ICU dll components
macro    (regexliteFindIcuDll
          _libs_to_find
          _append_to)

  string(REPLACE "." ";" _ver_comps "${ICU_VERSION}")
  # message(STATUS "_ver_comps = ${_ver_comps}")

  list(GET _ver_comps 0 _ver_major11)
  # message(STATUS "_ver_major11 = ${_ver_major11}")

  foreach(_iter_lib ${_libs_to_find})
      # message(STATUS "_iter_lib = ${_iter_lib}")
      get_filename_component(_base_name "${_iter_lib}" NAME_WE )
      # message(STATUS "_base_name = ${_base_name}")
      # message(STATUS "NAMES = ${_base_name}${_ver_major11}.dll ${_base_name}${ICU_VERSION}.dll ${_base_name}.dll")
      # message(STATUS "ICU_ICUINFO_EXECUTABLE = ${ICU_ICUINFO_EXECUTABLE}")
      unset(_iter_dll CACHE)
      find_file(_iter_dll
          NAMES "${_base_name}${_ver_major11}.dll" "${_base_name}${ICU_VERSION}.dll" "${_base_name}.dll"
          PATHS "${ICU_ICUINFO_EXECUTABLE}/..")
      # message(STATUS "_iter_dll = ${_iter_dll}")

      if (NOT _iter_dll)
          message(FATAL_ERROR "Cannot find ${_base_name} library in ICU package")
      endif ()
      list(APPEND ${_append_to} ${_iter_dll})
      unset(_iter_dll CACHE)
  endforeach()
  unset(_ver_comps)
endmacro()


# initialize this module
macro    (resqliteunInit
          resqliteun_use_mode)

    # default name
    if (NOT RESQLITEUN_INIT_NAME)
        set(RESQLITEUN_INIT_NAME "ReSqliteUn")
    endif ()

    find_package(ICU 58 REQUIRED
        COMPONENTS uc i18n dt)

    set(RESQLITEUN_LIBRARIES
        ${ICU_UC_LIBRARIES}
        ${ICU_I18N_LIBRARIES}
        ${ICU_DT_LIBRARIES})

    if (WIN32)
        regexliteFindIcuDll("${ICU_UC_LIBRARIES};${ICU_I18N_LIBRARIES};${ICU_DT_LIBRARIES}" REGEXLITE_DLLS)
        # message(STATUS "REGEXLITE_DLLS = ${REGEXLITE_DLLS}")
    endif()

    set(RESQLITEUN_INCLUDES
        ${ICU_INCLUDE_DIRS})

    # compose the list of headers and sources
    set(RESQLITEUN_HEADERS
        "resqliteun-names.h"
        "resqliteun-manager.h"
        "resqliteun-util.h"
        "resqliteun.h")
    set(RESQLITEUN_SOURCES
        "resqliteun-entry-points.cc"
        "resqliteun-manager.cc"
        "resqliteun-util.cc"
        "resqliteun.cc")

    pileSetSources(
        "${RESQLITEUN_INIT_NAME}"
        "${RESQLITEUN_HEADERS}"
        "${RESQLITEUN_SOURCES}")

    pileSetCommon(
        "${RESQLITEUN_INIT_NAME}"
        "0;0;1;d"
        "ON"
        "${resqliteun_use_mode}"
        ""
        "database"
        "sqlite")

endmacro ()
