#ifndef ONSEMWITHPLANNERTESTER_API_HPP
#define ONSEMWITHPLANNERTESTER_API_HPP

#include <onsem/common/exportsymbols/macro.hpp>

#if !defined(SWIG) && defined(onsemwithplannertester_EXPORTS)
# define ONSEMWITHPLANNERTESTER_API SEMANTIC_LIB_API_EXPORTS(onsemwithplannertester)
#elif !defined(SWIG)
# define ONSEMWITHPLANNERTESTER_API SEMANTIC_LIB_API(onsemwithplannertester)
#else
# define ONSEMWITHPLANNERTESTER_API
#endif

#endif // ONSEMWITHPLANNERTESTER_API_HPP
