set(RPI_RGB_LED_MATRIX_DIR CACHE PATH "Path to rpi-rgb-led-matrix root directory")

find_path(RpiRgbLedMatrix_INCLUDE_DIR
  NAMES led-matrix.h
  PATHS ${RPI_RGB_LED_MATRIX_DIR}/include
)

find_library(RpiRgbLedMatrix_LIBRARY
  NAMES rgbmatrix
  PATHS ${RPI_RGB_LED_MATRIX_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RpiRgbLedMatrix
  REQUIRED_VARS
    RpiRgbLedMatrix_INCLUDE_DIR
    RpiRgbLedMatrix_LIBRARY
)

mark_as_advanced(
  RpiRgbLedMatrix_INCLUDE_DIR
  RpiRgbLedMatrix_LIBRARY
)

if(RpiRgbLedMatrix_FOUND)
  add_library(RpiRgbLedMatrix::RpiRgbLedMatrix UNKNOWN IMPORTED)
  set_target_properties(RpiRgbLedMatrix::RpiRgbLedMatrix PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${RpiRgbLedMatrix_INCLUDE_DIR}
    IMPORTED_LOCATION ${RpiRgbLedMatrix_LIBRARY}
  )
endif()
