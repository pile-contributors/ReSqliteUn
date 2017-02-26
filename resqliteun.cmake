
# enable/disable cmake debug messages related to this pile
set (RESQLITEUN_DEBUG_MSG ON)

# make sure support code is present; no harm
# in including it twice; the user, however, should have used
# pileInclude() from pile_support.cmake module.
include(pile_support)

# initialize this module
macro    (resqliteunInit
          ref_cnt_use_mode)

    # default name
    if (NOT RESQLITEUN_INIT_NAME)
        set(RESQLITEUN_INIT_NAME "ReSqliteUn")
    endif ()

    # compose the list of headers and sources
    set(RESQLITEUN_HEADERS
        "resqliteun.h")
    set(RESQLITEUN_SOURCES
        "resqliteun.cc")

    pileSetSources(
        "${RESQLITEUN_INIT_NAME}"
        "${RESQLITEUN_HEADERS}"
        "${RESQLITEUN_SOURCES}")

    pileSetCommon(
        "${RESQLITEUN_INIT_NAME}"
        "0;0;1;d"
        "ON"
        "${ref_cnt_use_mode}"
        ""
        "category1"
        "tag1;tag2")

endmacro ()
