set(CTEST_PROJECT_NAME "vader")
set(CTEST_NIGHTLY_START_TIME "01:00:00 UTC")
set(CTEST_SUBMIT_URL "CDASH_URL/submit.php?project=${CTEST_PROJECT_NAME}")
set(CTEST_USE_LAUNCHERS 1)
set(ENV{CTEST_USE_LAUNCHERS_DEFAULT} 1)
set(CTEST_LABELS_FOR_SUBPROJECTS vader)
