
cmake_minimum_required (VERSION 2.8)
project (cuimg)

find_package(CUDA)
find_package(Boost REQUIRED)
find_package(OpenCV REQUIRED)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})


IF (NOT CUDA_FOUND)
  add_definitions(-DNO_CUDA)
  set(CUDA_LIBRARIES "")
#  ADD_EXECUTABLE(image2d tests/image2d.cu)
#  ADD_EXECUTABLE(host_image3d tests/host_image3d.cpp)
#  ADD_EXECUTABLE(shared_ptr tests/shared_ptr.cu)
#  ADD_EXECUTABLE(point2d tests/point2d.cu)
#  ADD_EXECUTABLE(obox2d tests/obox2d.cu)
#  ADD_EXECUTABLE(make_meta_gaussian make_meta_gaussian.cpp)
#  ADD_EXECUTABLE(improved_builtin tests/improved_builtin.cu)

#  SET_TARGET_PROPERTIES(image2d PROPERTIES LINKER_LANGUAGE C)
#  SET_TARGET_PROPERTIES(host_image3d PROPERTIES LINKER_LANGUAGE C)
#  SET_TARGET_PROPERTIES(shared_ptr PROPERTIES LINKER_LANGUAGE C)
#  SET_TARGET_PROPERTIES(point2d PROPERTIES LINKER_LANGUAGE C)
#  SET_TARGET_PROPERTIES(obox2d PROPERTIES LINKER_LANGUAGE C)
#  SET_TARGET_PROPERTIES(make_meta_gaussian PROPERTIES LINKER_LANGUAGE C)
#  SET_TARGET_PROPERTIES(improved_builtin PROPERTIES LINKER_LANGUAGE C)

ELSE (NOT CUDA_FOUND)
  CUDA_ADD_EXECUTABLE(image2d tests/image2d.cu)
  CUDA_ADD_EXECUTABLE(host_image3d tests/host_image3d.cpp)
  CUDA_ADD_EXECUTABLE(shared_ptr tests/shared_ptr.cu)
  CUDA_ADD_EXECUTABLE(point2d tests/point2d.cu)
  CUDA_ADD_EXECUTABLE(obox2d tests/obox2d.cu)
  CUDA_ADD_EXECUTABLE(make_meta_gaussian make_meta_gaussian.cpp)
  CUDA_ADD_EXECUTABLE(improved_builtin tests/improved_builtin.cu)
ENDIF (NOT CUDA_FOUND)

SET(SOURCES cuimg/video_capture.cpp
            cuimg/background_video_capture.cpp
            cuimg/load_image.cpp
            cuimg/save_image.cpp
            )

ADD_LIBRARY(cuimg ${SOURCES})

install(TARGETS cuimg DESTINATION lib)
install(TARGETS cuimg DESTINATION lib/Debug CONFIGURATIONS Debug)
install(TARGETS cuimg DESTINATION lib/Release CONFIGURATIONS Release)

install(DIRECTORY cuimg DESTINATION include
  FILES_MATCHING PATTERN "*.h")
install(DIRECTORY cuimg DESTINATION include
  FILES_MATCHING PATTERN "*.hpp")


ENABLE_TESTING()


TARGET_LINK_LIBRARIES(improved_builtin ${CUDA_LIBRARIES})
ADD_TEST(improved_builtin ${EXECUTABLE_OUTPUT_PATH}/improved_builtin)

TARGET_LINK_LIBRARIES(image2d ${CUDA_LIBRARIES})
ADD_TEST(image2d ${EXECUTABLE_OUTPUT_PATH}/image2d)

TARGET_LINK_LIBRARIES(host_image3d ${CUDA_LIBRARIES})
ADD_TEST(host_image3d ${EXECUTABLE_OUTPUT_PATH}/host_image3d)

TARGET_LINK_LIBRARIES(shared_ptr)
ADD_TEST(shared_ptr ${EXECUTABLE_OUTPUT_PATH}/shared_ptr)

TARGET_LINK_LIBRARIES(point2d)
ADD_TEST(point2d ${EXECUTABLE_OUTPUT_PATH}/point2d)

TARGET_LINK_LIBRARIES(obox2d)
ADD_TEST(obox2d ${EXECUTABLE_OUTPUT_PATH}/obox2d)
