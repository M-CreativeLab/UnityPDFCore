/* Header for class com_artifex_mupdf_fitz_Archive */
#ifndef _Included_mupdf_core_native
#define _Included_mupdf_core_native

#include <jni.h>
#include <mupdf/MuPDFWrapper.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT int GetPermissions_JNI(fz_context* ctx, fz_document* doc) {
	return GetPermissions(ctx, doc);
}

#ifdef __cplusplus
}
#endif
#endif
