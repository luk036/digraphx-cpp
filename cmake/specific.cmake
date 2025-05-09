set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

CPMAddPackage(
  NAME fmt
  GIT_TAG 10.2.1
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

CPMAddPackage(
  NAME Py2Cpp
  GIT_TAG 1.4.9
  GITHUB_REPOSITORY luk036/py2cpp
  OPTIONS "INSTALL_ONLY YES" # create an installable target
)

CPMAddPackage(
  NAME MyWheel
  GIT_TAG 1.0.3
  GITHUB_REPOSITORY luk036/mywheel-cpp
  OPTIONS "INSTALL_ONLY YES" # create an installable target
)

set(SPECIFIC_LIBS Threads::Threads MyWheel::MyWheel Py2Cpp::Py2Cpp fmt::fmt)
