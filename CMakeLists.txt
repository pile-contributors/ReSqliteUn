if (NOT RESQLITEUN_BUILD_MODE)
    set (RESQLITEUN_BUILD_MODE STATIC)
endif ()

include(pile_support)
pileInclude (ReSqliteUn)
resqliteunInit(${RESQLITEUN_BUILD_MODE})
