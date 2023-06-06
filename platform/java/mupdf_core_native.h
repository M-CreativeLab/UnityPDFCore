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

	JNIEXPORT int UnlockWithPassword_JNI(fz_context* ctx, fz_document* doc, const char* password) {
		return UnlockWithPassword(ctx, doc, password);
	}

	JNIEXPORT int CheckIfPasswordNeeded_JNI(fz_context* ctx, fz_document* doc) {
		return CheckIfPasswordNeeded(ctx, doc);
	}

	JNIEXPORT void ResetOutput_JNI(int stdoutFD, int stderrFD) {
		return ResetOutput(stdoutFD, stderrFD);
	}

	JNIEXPORT void WriteToFileDescriptor_JNI(int fileDescriptor, const char* text, int length) {
		return WriteToFileDescriptor(fileDescriptor, text, length);
	}

	JNIEXPORT void RedirectOutput_JNI(int* stdoutFD, int* stderrFD, const char* stdoutPipe, const char* stderrPipe) {
		return RedirectOutput(stdoutFD, stderrFD, stdoutPipe, stderrPipe);
	}

	JNIEXPORT int GetStructuredTextChar_JNI(fz_stext_char* character, int* out_c, int* out_color, float* out_origin_x, float* out_origin_y, float* out_size, float* out_ll_x, float* out_ll_y, float* out_ul_x, float* out_ul_y, float* out_ur_x, float* out_ur_y, float* out_lr_x, float* out_lr_y) {
		return GetStructuredTextChar(character, out_c, out_color, out_origin_x, out_origin_y, out_size, out_ll_x, out_ll_y, out_ul_x, out_ul_y, out_ur_x, out_ur_y, out_lr_x, out_lr_y);
	}

	JNIEXPORT int GetStructuredTextChars_JNI(fz_stext_line* line, fz_stext_char** out_chars) {
		return GetStructuredTextChars(line, out_chars);
	}

	JNIEXPORT int GetStructuredTextLine_JNI(fz_stext_line* line, int* out_wmode, float* out_x0, float* out_y0, float* out_x1, float* out_y1, float* out_x, float* out_y, int* out_char_count) {
		return GetStructuredTextLine(line, out_wmode, out_x0, out_y0, out_x1, out_y1, out_x, out_y, out_char_count);
	}

	JNIEXPORT int GetStructuredTextLines_JNI(fz_stext_block* block, fz_stext_line** out_lines) {
		return GetStructuredTextLines(block, out_lines);
	}

	JNIEXPORT int GetStructuredTextBlock_JNI(fz_stext_block* block, int* out_type, float* out_x0, float* out_y0, float* out_x1, float* out_y1, int* out_line_count) {
		return GetStructuredTextBlock(block, out_type, out_x0, out_y0, out_x1, out_y1, out_line_count);
	}

	JNIEXPORT int GetStructuredTextBlocks_JNI(fz_stext_page* page, fz_stext_block** out_blocks) {
		return GetStructuredTextBlocks(page, out_blocks);
	}

	JNIEXPORT int GetStructuredTextPageWithOCR_JNI(fz_context* ctx, fz_display_list* list, fz_stext_page** out_page, int* out_stext_block_count, float zoom, float x0, float y0, float x1, float y1, char* prefix, char* language, int callback(int)) {
		return GetStructuredTextPageWithOCR(ctx, list, out_page, out_stext_block_count, zoom, x0, y0, x1, y1, prefix, language, callback);
	}

	JNIEXPORT int GetStructuredTextPage_JNI(fz_context* ctx, fz_display_list* list, fz_stext_page** out_page, int* out_stext_block_count) {
		return GetStructuredTextPage(ctx, list, out_page, out_stext_block_count);
	}

	JNIEXPORT int DisposeStructuredTextPage_JNI(fz_context* ctx, fz_stext_page* page) {
		return DisposeStructuredTextPage(ctx, page);
	}

	JNIEXPORT int FinalizeDocumentWriter_JNI(fz_context* ctx, fz_document_writer* writ) {
		return FinalizeDocumentWriter(ctx, writ);
	}

	JNIEXPORT int WriteSubDisplayListAsPage_JNI(fz_context* ctx, fz_display_list* list, float x0, float y0, float x1, float y1, float zoom, fz_document_writer* writ) {
		return WriteSubDisplayListAsPage(ctx, list, x0, y0, x1, y1, zoom, writ);
	}

	JNIEXPORT int CreateDocumentWriter_JNI(fz_context* ctx, const char* file_name, int format, const fz_document_writer** out_document_writer) {
		return CreateDocumentWriter(ctx, file_name, format, out_document_writer);
	}

	JNIEXPORT int WriteImage_JNI(fz_context* ctx, fz_display_list* list, float x0, float y0, float x1, float y1, float zoom, int colorFormat, int output_format, const fz_buffer** out_buffer, const unsigned char** out_data, uint64_t* out_length) {
		return WriteImage(ctx, list, x0, y0, x1, y1, zoom, colorFormat, output_format, out_buffer, out_data, out_length);
	}

	JNIEXPORT int DisposeBuffer_JNI(fz_context* ctx, fz_buffer* buf) {
		return DisposeBuffer(ctx, buf);
	}

	JNIEXPORT int SaveImage_JNI(fz_context* ctx, fz_display_list* list, float x0, float y0, float x1, float y1, float zoom, int colorFormat, const char* file_name, int output_format) {
		return SaveImage(ctx, list, x0, y0, x1, y1, zoom, colorFormat, file_name, output_format);
	}

	JNIEXPORT int CloneContext_JNI(fz_context* ctx, int count, fz_context** out_contexts) {
		return CloneContext(ctx, count, out_contexts);
	}

	JNIEXPORT int RenderSubDisplayList_JNI(fz_context* ctx, fz_display_list* list, float x0, float y0, float x1, float y1, float zoom, int colorFormat, unsigned char* pixel_storage, fz_cookie* cookie) {
		return RenderSubDisplayList(ctx, list, x0, y0, x1, y1, zoom, colorFormat, pixel_storage, cookie);
	}

	JNIEXPORT int GetDisplayList_JNI(fz_context* ctx, fz_page* page, int annotations, fz_display_list** out_display_list, float* out_x0, float* out_y0, float* out_x1, float* out_y1) {
		return GetDisplayList(ctx, page, annotations, out_display_list, out_x0, out_y0, out_x1, out_y1);
	}

	JNIEXPORT int DisposeDisplayList_JNI(fz_context* ctx, fz_display_list* list) {
		return DisposeDisplayList(ctx, list);
	}

	JNIEXPORT int LoadPage_JNI(fz_context* ctx, fz_document* doc, int page_number, const fz_page** out_page, float* out_x, float* out_y, float* out_w, float* out_h) {
		return LoadPage(ctx, doc, page_number, out_page, out_x, out_y, out_w, out_h);
	}

	JNIEXPORT int DisposePage_JNI(fz_context* ctx, fz_page* page) {
		return DisposePage(ctx, page);
	}

	JNIEXPORT int CreateDocumentFromFile_JNI(fz_context* ctx, const char* file_name, int get_image_resolution, const fz_document** out_doc, int* out_page_count, float* out_image_xres, float* out_image_yres) {
		return CreateDocumentFromFile(ctx, file_name, get_image_resolution, out_doc, out_page_count, out_image_xres, out_image_yres);
	}

	JNIEXPORT int CreateDocumentFromStream_JNI(fz_context* ctx, const unsigned char* data, const uint64_t data_length, const char* file_type, int get_image_resolution, const fz_document** out_doc, const fz_stream** out_str, int* out_page_count, float* out_image_xres, float* out_image_yres) {
		return CreateDocumentFromStream(ctx, data, data_length, file_type, get_image_resolution, out_doc, out_str, out_page_count, out_image_xres, out_image_yres);
	}

	JNIEXPORT int DisposeStream_JNI(fz_context* ctx, fz_stream* str) {
		return DisposeStream(ctx, str);
	}

	JNIEXPORT int DisposeDocument_JNI(fz_context* ctx, fz_document* doc) {
		return DisposeDocument(ctx, doc);
	}

	JNIEXPORT uint64_t GetCurrentStoreSize_JNI(const fz_context* ctx) {
		return GetCurrentStoreSize(ctx);
	}

	JNIEXPORT uint64_t GetMaxStoreSize_JNI(const fz_context* ctx) {
		return GetMaxStoreSize(ctx);
	}

	JNIEXPORT int ShrinkStore_JNI(fz_context* ctx, unsigned int perc) {
		return ShrinkStore(ctx, perc);
	}

	JNIEXPORT void EmptyStore_JNI(fz_context* ctx) {
		return EmptyStore(ctx);
	}

	JNIEXPORT int CreateContext_JNI(uint64_t store_size, const fz_context** out_ctx) {
		return CreateContext(store_size, out_ctx);
	}

	JNIEXPORT int DisposeContext_JNI(fz_context* ctx) {
		return DisposeContext(ctx);
	}

	JNIEXPORT void SetLayoutParameters_JNI(fz_context* ctx, fz_document* doc, float w, float h, float em) {
		return SetLayoutParameters(ctx, doc, w, h, em);
	}

	JNIEXPORT void InstallLoadSystemFonts_JNI(fz_context* ctx) {
		return InstallLoadSystemFonts(ctx);
	}

#ifdef __cplusplus
}
#endif
#endif
