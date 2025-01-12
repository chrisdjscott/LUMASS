PROJECT(rimageio)


INCLUDE_DIRECTORIES(
	${PostgreSQL_INCLUDE_DIRS}
	${rimageio_SOURCE_DIR}
	${shared_SOURCE_DIR}
	${RASDAMAN_INCLUDE_DIR}
	${RASDAMAN_INCLUDE_DIR}/raslib
	${RASDAMAN_SOURCE_DIR}/applications/rasgeo
	${RASDAMAN_SOURCE_DIR}
	${OTB_INCLUDE_DIRS}
	${GDALRATImageIO_SOURCE_DIR}
        ${GDALRATImageIO_BINARY_DIR}
)

LINK_DIRECTORIES(
	${PostgreSQL_LIBRARY_DIRS}
	#${rimageio_BINARY_DIR}/lib
	${OTB_LIBRARY_DIRS}
	${RASDAMAN_LIBRARIES_DIR}
)

LIST(APPEND RASLIBS clientcomm rasodmg conversion 
	clientcomm rasodmg raslib 
	netpbm jpeg png tiff crypto network m z)

# flags for the rasdaman library (i.e. for initiating the explicit template instantiation)
ADD_DEFINITIONS(-DLINUX -DEARLY_TEMPLATE -DONCRB -g 
		-DCOMPDATE -DRMANVERSION -DBASEDBSTRING
		-DORFEOMAN)

# the RMANDEBUG symbol is set if rasdaman was compiled in debug mode		
if(RASDEBUG)
        ADD_DEFINITIONS(-DRMANDEBUG)
endif()


ADD_LIBRARY(RasImageIO SHARED 
	${RASDAMAN_SOURCE_DIR}/applications/rasgeo/RasdamanHelper2.cc
	${RASDAMAN_SOURCE_DIR}/applications/rasgeo/RasdamanConnector.cc
	${rimageio_SOURCE_DIR}/otbRasdamanImageIO.cxx
	${rimageio_SOURCE_DIR}/otbRasdamanImageIOFactory.cxx
	${rimageio_SOURCE_DIR}/otbRasdamanImageReader.txx
)
set_property(TARGET RasImageIO PROPERTY POSITION_INDEPENDENT_CODE 1)
TARGET_LINK_LIBRARIES(RasImageIO ${RASLIBS} ${PostgreSQL_LIBRARIES} OTBCommon OTBIO)	
ADD_DEPENDENCIES(RasImageIO OTBGDALRATImageIO)

FILE(GLOB RIO_HEADER  
	${RASDAMAN_SOURCE_DIR}/applications/rasgeo/RasdamanHelper2.hh
	${RASDAMAN_SOURCE_DIR}/applications/rasgeo/RasdamanConnector.hh
	#${rimageio_SOURCE_DIR}/.h
	${rimageio_SOURCE_DIR}/.hh
)
	
install(FILES ${RIO_HEADER} DESTINATION include)
install(TARGETS RasImageIO LIBRARY DESTINATION lib)
#install(TARGETS RasImageIO ARCHIVE DESTINATION lib)

