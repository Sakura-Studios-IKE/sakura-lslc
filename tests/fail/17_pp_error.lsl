// Expected: #error triggers an error from the preprocessor
#ifndef FEATURE_FLAG
#error FEATURE_FLAG must be defined on the command line: -DFEATURE_FLAG=1
#endif

default { state_entry() {} }
