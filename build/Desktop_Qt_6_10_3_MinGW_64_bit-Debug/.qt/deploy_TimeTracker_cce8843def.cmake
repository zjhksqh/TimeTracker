include("D:/QT/lujing/TimeTracker/build/Desktop_Qt_6_10_3_MinGW_64_bit-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/TimeTracker-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "D:/QT/lujing/TimeTracker/build/Desktop_Qt_6_10_3_MinGW_64_bit-Debug/TimeTracker.exe"
    GENERATE_QT_CONF
)
