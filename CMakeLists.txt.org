cmake_minimum_required(VERSION 2.8.3)
project(kaldi)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
  roscpp rospy std_msgs 
)

find_package(CUDA REQUIRED)

include(FindCUDA)

set(CMAKE_CXX_FLAGS "-msse -msse2 -Wall -pthread -DKALDI_DOUBLEPRECISION=0 -DHAVE_POSIX_MEMALIGN -Wno-sign-compare -Wno-unused-local-typedefs -Winit-self -DHAVE_EXECINFO_H=1 -rdynamic  -fPIC --verbose --machine 64 -DHAVE_CXXABI_H -DHAVE_ATLAS -DHAVE_CUDA -DHAVE_SPEEX  -O3 -DNDEBUG")

# Pass options to NVCC
set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-O3 -gencode arch=compute_50,code=sm_50  -Xcompiler)
 



catkin_package(
  INCLUDE_DIRS include
  LIBRARIES kaldi
  CATKIN_DEPENDS roscpp rospy std_msgs
  DEPENDS system_lib
)

###########
## Build ##
###########

include_directories(
  /usr/local/include /usr/local/cuda/include ~/src/kaldi/src ~/src/kaldi/src/online2 ~/src/kaldi/src/ivector ~/src/kaldi/src/nnet2 ~/src/kaldi/src/lat ~/src/kaldi/src/decoder ~/src/kaldi/src/cudamatrix ~/src/kaldi/src/feat ~/src/kaldi/src/transform ~/src/kaldi/src/gmm ~/src/kaldi/src/thread ~/src/kaldi/src/hmm ~/src/kaldi/src/tree ~/src/kaldi/src/matrix ~/src/kaldi/src/util ~/src/kaldi/src/base  ~/src/kaldi/tools/ATLAS/include ~/src/kaldi/tools/{*} ~/src/kaldi/tools/{*}/include ${catkin_INCLUDE_DIRS}
)

link_directories(/usr/local/lib /usr/lib /usr/local/cuda/lib64 ~/src/kaldi/src/online2 ~/src/kaldi/src/ivector ~/src/kaldi/src/nnet2 ~/src/kaldi/src/lat ~/src/kaldi/src/decoder  ~/src/kaldi/src/cudamatrix ~/src/kaldi/src/feat ~/src/kaldi/src/transform ~/src/kaldi/src/gmm ~/src/kaldi/src/thread ~/src/kaldi/src/hmm ~/src/kaldi/src/tree ~/src/kaldi/src/matrix ~/src/kaldi/src/util ~/src/kaldi/src/base } 
)

## Declare a cpp executable
add_executable(kaldi src/kaldi.cpp)

target_link_libraries(kaldi  ${catkin_LIBRARIES}  
   kaldi-online2.a kaldi-ivector.a kaldi-nnet2.a kaldi-lat.a 
   kaldi-decoder.a  kaldi-cudamatrix.a kaldi-feat.a kaldi-transform.a 
   kaldi-gmm.a  kaldi-thread.a kaldi-hmm.a kaldi-tree.a kaldi-matrix.a 
   kaldi-util.a kaldi-base.a libfst.a  libfstscript.a libcublas.so libcudart.so 
   libatlas.so.3 libf77blas.so.3 libcblas.so.3 liblapack_atlas.so.3 libdl.so
 )

#############
## Install ##
#############
## Mark executables and/or libraries for installation
install(TARGETS kaldi
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

## Mark cpp header files for installation
# install(DIRECTORY include/${PROJECT_NAME}/
#   DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
#   FILES_MATCHING PATTERN "*.h"
#   PATTERN ".svn" EXCLUDE
# )

## Mark other files for installation (e.g. launch and bag files, etc.)
# install(FILES
#   # myfile1
#   # myfile2
#   DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
# )

#############
## Testing ##
#############

## Add gtest based cpp test target and link libraries
# catkin_add_gtest(${PROJECT_NAME}-test test/test_kaldi.cpp)
# if(TARGET ${PROJECT_NAME}-test)
#   target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
# endif()

## Add folders to be run by python nosetests
# catkin_add_nosetests(test)
