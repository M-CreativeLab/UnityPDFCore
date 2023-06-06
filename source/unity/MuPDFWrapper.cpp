#include <mupdf/fitz.h>
#include <mupdf/fitz/display-list.h>
#include <mupdf/fitz/store.h>
#include <mupdf/ucdn.h>
#include <mupdf/MuPDFWrapper.h>

#include <stdio.h>
#include <stdlib.h>
#include <mutex>

#include <iostream>
#include <fcntl.h>

#if defined _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

mutex_holder global_mutex;

fz_pixmap*
new_pixmap_with_data(fz_context* ctx, fz_colorspace* colorspace, int w, int h, fz_separations* seps, int alpha, unsigned char* pixel_storage)
{
	int stride;
	int s = fz_count_active_separations(ctx, seps);
	if (!colorspace && s == 0) alpha = 1;
	stride = (fz_colorspace_n(ctx, colorspace) + s + alpha) * w;
	return fz_new_pixmap_with_data(ctx, colorspace, w, h, seps, alpha, stride, pixel_storage);
}

fz_pixmap*
new_pixmap_with_bbox_and_data(fz_context* ctx, fz_colorspace* colorspace, fz_irect bbox, fz_separations* seps, int alpha, unsigned char* pixel_storage)
{
	fz_pixmap* pixmap;
	pixmap = new_pixmap_with_data(ctx, colorspace, bbox.x1 - bbox.x0, bbox.y1 - bbox.y0, seps, alpha, pixel_storage);
	pixmap->x = bbox.x0;
	pixmap->y = bbox.y0;
	return pixmap;
}

fz_pixmap*
new_pixmap_from_display_list_with_separations_bbox_and_data(fz_context* ctx, fz_display_list* list, fz_rect rect, fz_matrix ctm, fz_colorspace* cs, fz_separations* seps, int alpha, unsigned char* pixel_storage, fz_cookie* cookie)
{
	fz_irect bbox;
	fz_pixmap* pix;
	fz_device* dev = NULL;

	fz_var(dev);

	rect = fz_transform_rect(rect, ctm);
	bbox = fz_round_rect(rect);

	pix = new_pixmap_with_bbox_and_data(ctx, cs, bbox, seps, alpha, pixel_storage);
	if (alpha)
		fz_clear_pixmap(ctx, pix);
	else
		fz_clear_pixmap_with_value(ctx, pix, 0xFF);

	fz_try(ctx)
	{
		dev = fz_new_draw_device(ctx, ctm, pix);
		fz_run_display_list(ctx, list, dev, fz_identity, fz_infinite_rect, cookie);
		fz_close_device(ctx, dev);
	}
	fz_always(ctx)
	{
		fz_drop_device(ctx, dev);
	}
	fz_catch(ctx)
	{
		fz_drop_pixmap(ctx, pix);
		fz_rethrow(ctx);
	}

	return pix;
}

fz_pixmap*
new_pixmap_from_display_list_with_separations_bbox(fz_context* ctx, fz_display_list* list, fz_rect rect, fz_matrix ctm, fz_colorspace* cs, fz_separations* seps, int alpha)
{
	fz_irect bbox;
	fz_pixmap* pix;
	fz_device* dev = NULL;

	fz_var(dev);

	rect = fz_transform_rect(rect, ctm);
	bbox = fz_round_rect(rect);

	pix = fz_new_pixmap_with_bbox(ctx, cs, bbox, seps, alpha);
	if (alpha)
		fz_clear_pixmap(ctx, pix);
	else
		fz_clear_pixmap_with_value(ctx, pix, 0xFF);

	fz_try(ctx)
	{
		dev = fz_new_draw_device(ctx, ctm, pix);
		fz_run_display_list(ctx, list, dev, fz_identity, fz_infinite_rect, NULL);
		fz_close_device(ctx, dev);
	}
	fz_always(ctx)
	{
		fz_drop_device(ctx, dev);
	}
	fz_catch(ctx)
	{
		fz_drop_pixmap(ctx, pix);
		fz_rethrow(ctx);
	}

	return pix;
}

void lock_mutex(void* user, int lock)
{
	mutex_holder* mutex = (mutex_holder*)user;

	switch (lock)
	{
	case 0:
		mutex->mutex0.lock();
		break;
	case 1:
		mutex->mutex1.lock();
		break;
	case 2:
		mutex->mutex2.lock();
		break;
	default:
		mutex->mutex3.lock();
		break;
	}
}

void unlock_mutex(void* user, int lock)
{
	mutex_holder* mutex = (mutex_holder*)user;

	switch (lock)
	{
	case 0:
		mutex->mutex0.unlock();
		break;
	case 1:
		mutex->mutex1.unlock();
		break;
	case 2:
		mutex->mutex2.unlock();
		break;
	default:
		mutex->mutex3.unlock();
		break;
	}
}

// Font for Android
static fz_font *load_noto(fz_context *ctx, const char *a, const char *b, const char *c, int idx)
{
	char buf[500];
	fz_font *font = NULL;
	fz_try(ctx)
	{
		fz_snprintf(buf, sizeof buf, "/system/fonts/%s%s%s.ttf", a, b, c);
		if (!fz_file_exists(ctx, buf))
			fz_snprintf(buf, sizeof buf, "/system/fonts/%s%s%s.otf", a, b, c);
		if (!fz_file_exists(ctx, buf))
			fz_snprintf(buf, sizeof buf, "/system/fonts/%s%s%s.ttc", a, b, c);
		if (fz_file_exists(ctx, buf))
			font = fz_new_font_from_file(ctx, NULL, buf, idx, 0);
	}
	fz_catch(ctx)
		return NULL;
	return font;
}

static fz_font *load_noto_cjk(fz_context *ctx, int lang)
{
	fz_font *font = load_noto(ctx, "NotoSerif", "CJK", "-Regular", lang);
	if (!font) font = load_noto(ctx, "NotoSans", "CJK", "-Regular", lang);
	if (!font) font = load_noto(ctx, "DroidSans", "Fallback", "", 0);
	return font;
}

static fz_font *load_noto_arabic(fz_context *ctx)
{
	fz_font *font = load_noto(ctx, "Noto", "Naskh", "-Regular", 0);
	if (!font) font = load_noto(ctx, "Noto", "NaskhArabic", "-Regular", 0);
	if (!font) font = load_noto(ctx, "Droid", "Naskh", "-Regular", 0);
	if (!font) font = load_noto(ctx, "NotoSerif", "Arabic", "-Regular", 0);
	if (!font) font = load_noto(ctx, "NotoSans", "Arabic", "-Regular", 0);
	if (!font) font = load_noto(ctx, "DroidSans", "Arabic", "-Regular", 0);
	return font;
}

static fz_font *load_noto_try(fz_context *ctx, const char *stem)
{
	fz_font *font = load_noto(ctx, "NotoSerif", stem, "-Regular", 0);
	if (!font) font = load_noto(ctx, "NotoSans", stem, "-Regular", 0);
	if (!font) font = load_noto(ctx, "DroidSans", stem, "-Regular", 0);
	return font;
}

enum { JP, KR, SC, TC };

fz_font *load_droid_fallback_font(fz_context *ctx, int script, int language, int serif, int bold, int italic)
{
	switch (script)
	{
	default:
	case UCDN_SCRIPT_COMMON:
	case UCDN_SCRIPT_INHERITED:
	case UCDN_SCRIPT_UNKNOWN:
		return NULL;

	case UCDN_SCRIPT_HANGUL: return load_noto_cjk(ctx, KR);
	case UCDN_SCRIPT_HIRAGANA: return load_noto_cjk(ctx, JP);
	case UCDN_SCRIPT_KATAKANA: return load_noto_cjk(ctx, JP);
	case UCDN_SCRIPT_BOPOMOFO: return load_noto_cjk(ctx, TC);
	case UCDN_SCRIPT_HAN:
		switch (language)
		{
		case FZ_LANG_ja: return load_noto_cjk(ctx, JP);
		case FZ_LANG_ko: return load_noto_cjk(ctx, KR);
		case FZ_LANG_zh_Hans: return load_noto_cjk(ctx, SC);
		default:
		case FZ_LANG_zh_Hant: return load_noto_cjk(ctx, TC);
		}

	case UCDN_SCRIPT_LATIN:
	case UCDN_SCRIPT_GREEK:
	case UCDN_SCRIPT_CYRILLIC:
		return load_noto_try(ctx, "");
	case UCDN_SCRIPT_ARABIC:
		return load_noto_arabic(ctx);

	case UCDN_SCRIPT_ARMENIAN: return load_noto_try(ctx, "Armenian");
	case UCDN_SCRIPT_HEBREW: return load_noto_try(ctx, "Hebrew");
	case UCDN_SCRIPT_SYRIAC: return load_noto_try(ctx, "Syriac");
	case UCDN_SCRIPT_THAANA: return load_noto_try(ctx, "Thaana");
	case UCDN_SCRIPT_DEVANAGARI: return load_noto_try(ctx, "Devanagari");
	case UCDN_SCRIPT_BENGALI: return load_noto_try(ctx, "Bengali");
	case UCDN_SCRIPT_GURMUKHI: return load_noto_try(ctx, "Gurmukhi");
	case UCDN_SCRIPT_GUJARATI: return load_noto_try(ctx, "Gujarati");
	case UCDN_SCRIPT_ORIYA: return load_noto_try(ctx, "Oriya");
	case UCDN_SCRIPT_TAMIL: return load_noto_try(ctx, "Tamil");
	case UCDN_SCRIPT_TELUGU: return load_noto_try(ctx, "Telugu");
	case UCDN_SCRIPT_KANNADA: return load_noto_try(ctx, "Kannada");
	case UCDN_SCRIPT_MALAYALAM: return load_noto_try(ctx, "Malayalam");
	case UCDN_SCRIPT_SINHALA: return load_noto_try(ctx, "Sinhala");
	case UCDN_SCRIPT_THAI: return load_noto_try(ctx, "Thai");
	case UCDN_SCRIPT_LAO: return load_noto_try(ctx, "Lao");
	case UCDN_SCRIPT_TIBETAN: return load_noto_try(ctx, "Tibetan");
	case UCDN_SCRIPT_MYANMAR: return load_noto_try(ctx, "Myanmar");
	case UCDN_SCRIPT_GEORGIAN: return load_noto_try(ctx, "Georgian");
	case UCDN_SCRIPT_ETHIOPIC: return load_noto_try(ctx, "Ethiopic");
	case UCDN_SCRIPT_CHEROKEE: return load_noto_try(ctx, "Cherokee");
	case UCDN_SCRIPT_CANADIAN_ABORIGINAL: return load_noto_try(ctx, "CanadianAboriginal");
	case UCDN_SCRIPT_OGHAM: return load_noto_try(ctx, "Ogham");
	case UCDN_SCRIPT_RUNIC: return load_noto_try(ctx, "Runic");
	case UCDN_SCRIPT_KHMER: return load_noto_try(ctx, "Khmer");
	case UCDN_SCRIPT_MONGOLIAN: return load_noto_try(ctx, "Mongolian");
	case UCDN_SCRIPT_YI: return load_noto_try(ctx, "Yi");
	case UCDN_SCRIPT_OLD_ITALIC: return load_noto_try(ctx, "OldItalic");
	case UCDN_SCRIPT_GOTHIC: return load_noto_try(ctx, "Gothic");
	case UCDN_SCRIPT_DESERET: return load_noto_try(ctx, "Deseret");
	case UCDN_SCRIPT_TAGALOG: return load_noto_try(ctx, "Tagalog");
	case UCDN_SCRIPT_HANUNOO: return load_noto_try(ctx, "Hanunoo");
	case UCDN_SCRIPT_BUHID: return load_noto_try(ctx, "Buhid");
	case UCDN_SCRIPT_TAGBANWA: return load_noto_try(ctx, "Tagbanwa");
	case UCDN_SCRIPT_LIMBU: return load_noto_try(ctx, "Limbu");
	case UCDN_SCRIPT_TAI_LE: return load_noto_try(ctx, "TaiLe");
	case UCDN_SCRIPT_LINEAR_B: return load_noto_try(ctx, "LinearB");
	case UCDN_SCRIPT_UGARITIC: return load_noto_try(ctx, "Ugaritic");
	case UCDN_SCRIPT_SHAVIAN: return load_noto_try(ctx, "Shavian");
	case UCDN_SCRIPT_OSMANYA: return load_noto_try(ctx, "Osmanya");
	case UCDN_SCRIPT_CYPRIOT: return load_noto_try(ctx, "Cypriot");
	case UCDN_SCRIPT_BUGINESE: return load_noto_try(ctx, "Buginese");
	case UCDN_SCRIPT_COPTIC: return load_noto_try(ctx, "Coptic");
	case UCDN_SCRIPT_NEW_TAI_LUE: return load_noto_try(ctx, "NewTaiLue");
	case UCDN_SCRIPT_GLAGOLITIC: return load_noto_try(ctx, "Glagolitic");
	case UCDN_SCRIPT_TIFINAGH: return load_noto_try(ctx, "Tifinagh");
	case UCDN_SCRIPT_SYLOTI_NAGRI: return load_noto_try(ctx, "SylotiNagri");
	case UCDN_SCRIPT_OLD_PERSIAN: return load_noto_try(ctx, "OldPersian");
	case UCDN_SCRIPT_KHAROSHTHI: return load_noto_try(ctx, "Kharoshthi");
	case UCDN_SCRIPT_BALINESE: return load_noto_try(ctx, "Balinese");
	case UCDN_SCRIPT_CUNEIFORM: return load_noto_try(ctx, "Cuneiform");
	case UCDN_SCRIPT_PHOENICIAN: return load_noto_try(ctx, "Phoenician");
	case UCDN_SCRIPT_PHAGS_PA: return load_noto_try(ctx, "PhagsPa");
	case UCDN_SCRIPT_NKO: return load_noto_try(ctx, "NKo");
	case UCDN_SCRIPT_SUNDANESE: return load_noto_try(ctx, "Sundanese");
	case UCDN_SCRIPT_LEPCHA: return load_noto_try(ctx, "Lepcha");
	case UCDN_SCRIPT_OL_CHIKI: return load_noto_try(ctx, "OlChiki");
	case UCDN_SCRIPT_VAI: return load_noto_try(ctx, "Vai");
	case UCDN_SCRIPT_SAURASHTRA: return load_noto_try(ctx, "Saurashtra");
	case UCDN_SCRIPT_KAYAH_LI: return load_noto_try(ctx, "KayahLi");
	case UCDN_SCRIPT_REJANG: return load_noto_try(ctx, "Rejang");
	case UCDN_SCRIPT_LYCIAN: return load_noto_try(ctx, "Lycian");
	case UCDN_SCRIPT_CARIAN: return load_noto_try(ctx, "Carian");
	case UCDN_SCRIPT_LYDIAN: return load_noto_try(ctx, "Lydian");
	case UCDN_SCRIPT_CHAM: return load_noto_try(ctx, "Cham");
	case UCDN_SCRIPT_TAI_THAM: return load_noto_try(ctx, "TaiTham");
	case UCDN_SCRIPT_TAI_VIET: return load_noto_try(ctx, "TaiViet");
	case UCDN_SCRIPT_AVESTAN: return load_noto_try(ctx, "Avestan");
	case UCDN_SCRIPT_EGYPTIAN_HIEROGLYPHS: return load_noto_try(ctx, "EgyptianHieroglyphs");
	case UCDN_SCRIPT_SAMARITAN: return load_noto_try(ctx, "Samaritan");
	case UCDN_SCRIPT_LISU: return load_noto_try(ctx, "Lisu");
	case UCDN_SCRIPT_BAMUM: return load_noto_try(ctx, "Bamum");
	case UCDN_SCRIPT_JAVANESE: return load_noto_try(ctx, "Javanese");
	case UCDN_SCRIPT_MEETEI_MAYEK: return load_noto_try(ctx, "MeeteiMayek");
	case UCDN_SCRIPT_IMPERIAL_ARAMAIC: return load_noto_try(ctx, "ImperialAramaic");
	case UCDN_SCRIPT_OLD_SOUTH_ARABIAN: return load_noto_try(ctx, "OldSouthArabian");
	case UCDN_SCRIPT_INSCRIPTIONAL_PARTHIAN: return load_noto_try(ctx, "InscriptionalParthian");
	case UCDN_SCRIPT_INSCRIPTIONAL_PAHLAVI: return load_noto_try(ctx, "InscriptionalPahlavi");
	case UCDN_SCRIPT_OLD_TURKIC: return load_noto_try(ctx, "OldTurkic");
	case UCDN_SCRIPT_KAITHI: return load_noto_try(ctx, "Kaithi");
	case UCDN_SCRIPT_BATAK: return load_noto_try(ctx, "Batak");
	case UCDN_SCRIPT_BRAHMI: return load_noto_try(ctx, "Brahmi");
	case UCDN_SCRIPT_MANDAIC: return load_noto_try(ctx, "Mandaic");
	case UCDN_SCRIPT_CHAKMA: return load_noto_try(ctx, "Chakma");
	case UCDN_SCRIPT_MIAO: return load_noto_try(ctx, "Miao");
	case UCDN_SCRIPT_MEROITIC_CURSIVE: return load_noto_try(ctx, "Meroitic");
	case UCDN_SCRIPT_MEROITIC_HIEROGLYPHS: return load_noto_try(ctx, "Meroitic");
	case UCDN_SCRIPT_SHARADA: return load_noto_try(ctx, "Sharada");
	case UCDN_SCRIPT_SORA_SOMPENG: return load_noto_try(ctx, "SoraSompeng");
	case UCDN_SCRIPT_TAKRI: return load_noto_try(ctx, "Takri");
	case UCDN_SCRIPT_BASSA_VAH: return load_noto_try(ctx, "BassaVah");
	case UCDN_SCRIPT_CAUCASIAN_ALBANIAN: return load_noto_try(ctx, "CaucasianAlbanian");
	case UCDN_SCRIPT_DUPLOYAN: return load_noto_try(ctx, "Duployan");
	case UCDN_SCRIPT_ELBASAN: return load_noto_try(ctx, "Elbasan");
	case UCDN_SCRIPT_GRANTHA: return load_noto_try(ctx, "Grantha");
	case UCDN_SCRIPT_KHOJKI: return load_noto_try(ctx, "Khojki");
	case UCDN_SCRIPT_KHUDAWADI: return load_noto_try(ctx, "Khudawadi");
	case UCDN_SCRIPT_LINEAR_A: return load_noto_try(ctx, "LinearA");
	case UCDN_SCRIPT_MAHAJANI: return load_noto_try(ctx, "Mahajani");
	case UCDN_SCRIPT_MANICHAEAN: return load_noto_try(ctx, "Manichaean");
	case UCDN_SCRIPT_MENDE_KIKAKUI: return load_noto_try(ctx, "MendeKikakui");
	case UCDN_SCRIPT_MODI: return load_noto_try(ctx, "Modi");
	case UCDN_SCRIPT_MRO: return load_noto_try(ctx, "Mro");
	case UCDN_SCRIPT_NABATAEAN: return load_noto_try(ctx, "Nabataean");
	case UCDN_SCRIPT_OLD_NORTH_ARABIAN: return load_noto_try(ctx, "OldNorthArabian");
	case UCDN_SCRIPT_OLD_PERMIC: return load_noto_try(ctx, "OldPermic");
	case UCDN_SCRIPT_PAHAWH_HMONG: return load_noto_try(ctx, "PahawhHmong");
	case UCDN_SCRIPT_PALMYRENE: return load_noto_try(ctx, "Palmyrene");
	case UCDN_SCRIPT_PAU_CIN_HAU: return load_noto_try(ctx, "PauCinHau");
	case UCDN_SCRIPT_PSALTER_PAHLAVI: return load_noto_try(ctx, "PsalterPahlavi");
	case UCDN_SCRIPT_SIDDHAM: return load_noto_try(ctx, "Siddham");
	case UCDN_SCRIPT_TIRHUTA: return load_noto_try(ctx, "Tirhuta");
	case UCDN_SCRIPT_WARANG_CITI: return load_noto_try(ctx, "WarangCiti");
	case UCDN_SCRIPT_AHOM: return load_noto_try(ctx, "Ahom");
	case UCDN_SCRIPT_ANATOLIAN_HIEROGLYPHS: return load_noto_try(ctx, "AnatolianHieroglyphs");
	case UCDN_SCRIPT_HATRAN: return load_noto_try(ctx, "Hatran");
	case UCDN_SCRIPT_MULTANI: return load_noto_try(ctx, "Multani");
	case UCDN_SCRIPT_OLD_HUNGARIAN: return load_noto_try(ctx, "OldHungarian");
	case UCDN_SCRIPT_SIGNWRITING: return load_noto_try(ctx, "Signwriting");
	case UCDN_SCRIPT_ADLAM: return load_noto_try(ctx, "Adlam");
	case UCDN_SCRIPT_BHAIKSUKI: return load_noto_try(ctx, "Bhaiksuki");
	case UCDN_SCRIPT_MARCHEN: return load_noto_try(ctx, "Marchen");
	case UCDN_SCRIPT_NEWA: return load_noto_try(ctx, "Newa");
	case UCDN_SCRIPT_OSAGE: return load_noto_try(ctx, "Osage");
	case UCDN_SCRIPT_TANGUT: return load_noto_try(ctx, "Tangut");
	case UCDN_SCRIPT_MASARAM_GONDI: return load_noto_try(ctx, "MasaramGondi");
	case UCDN_SCRIPT_NUSHU: return load_noto_try(ctx, "Nushu");
	case UCDN_SCRIPT_SOYOMBO: return load_noto_try(ctx, "Soyombo");
	case UCDN_SCRIPT_ZANABAZAR_SQUARE: return load_noto_try(ctx, "ZanabazarSquare");
	case UCDN_SCRIPT_DOGRA: return load_noto_try(ctx, "Dogra");
	case UCDN_SCRIPT_GUNJALA_GONDI: return load_noto_try(ctx, "GunjalaGondi");
	case UCDN_SCRIPT_HANIFI_ROHINGYA: return load_noto_try(ctx, "HanifiRohingya");
	case UCDN_SCRIPT_MAKASAR: return load_noto_try(ctx, "Makasar");
	case UCDN_SCRIPT_MEDEFAIDRIN: return load_noto_try(ctx, "Medefaidrin");
	case UCDN_SCRIPT_OLD_SOGDIAN: return load_noto_try(ctx, "OldSogdian");
	case UCDN_SCRIPT_SOGDIAN: return load_noto_try(ctx, "Sogdian");
	case UCDN_SCRIPT_ELYMAIC: return load_noto_try(ctx, "Elymaic");
	case UCDN_SCRIPT_NANDINAGARI: return load_noto_try(ctx, "Nandinagari");
	case UCDN_SCRIPT_NYIAKENG_PUACHUE_HMONG: return load_noto_try(ctx, "NyiakengPuachueHmong");
	case UCDN_SCRIPT_WANCHO: return load_noto_try(ctx, "Wancho");
	case UCDN_SCRIPT_CHORASMIAN: return load_noto_try(ctx, "Chorasmian");
	case UCDN_SCRIPT_DIVES_AKURU: return load_noto_try(ctx, "DivesAkuru");
	case UCDN_SCRIPT_KHITAN_SMALL_SCRIPT: return load_noto_try(ctx, "KhitanSmallScript");
	case UCDN_SCRIPT_YEZIDI: return load_noto_try(ctx, "Yezidi");
	case UCDN_SCRIPT_VITHKUQI: return load_noto_try(ctx, "Vithkuqi");
	case UCDN_SCRIPT_OLD_UYGHUR: return load_noto_try(ctx, "OldUyghur");
	case UCDN_SCRIPT_CYPRO_MINOAN: return load_noto_try(ctx, "CyproMinoan");
	case UCDN_SCRIPT_TANGSA: return load_noto_try(ctx, "Tangsa");
	case UCDN_SCRIPT_TOTO: return load_noto_try(ctx, "Toto");
	}
	return NULL;
}

fz_font *load_droid_cjk_font(fz_context *ctx, const char *name, int ros, int serif)
{
	switch (ros)
	{
	case FZ_ADOBE_CNS: return load_noto_cjk(ctx, TC);
	case FZ_ADOBE_GB: return load_noto_cjk(ctx, SC);
	case FZ_ADOBE_JAPAN: return load_noto_cjk(ctx, JP);
	case FZ_ADOBE_KOREA: return load_noto_cjk(ctx, KR);
	}
	return NULL;
}

fz_font *load_droid_font(fz_context *ctx, const char *name, int bold, int italic, int needs_exact_metrics)
{
	return NULL;
}

extern "C"
{
	DLL_PUBLIC int GetPermissions(fz_context* ctx, fz_document* doc)
	{
		int tbr = 0;

		if (fz_has_permission(ctx, doc, FZ_PERMISSION_PRINT))
		{
			tbr = tbr | 1;
		}

		if (fz_has_permission(ctx, doc, FZ_PERMISSION_COPY))
		{
			tbr = tbr | 2;
		}

		if (fz_has_permission(ctx, doc, FZ_PERMISSION_EDIT))
		{
			tbr = tbr | 4;
		}

		if (fz_has_permission(ctx, doc, FZ_PERMISSION_ANNOTATE))
		{
			tbr = tbr | 8;
		}

		return tbr;
	}

	DLL_PUBLIC int UnlockWithPassword(fz_context* ctx, fz_document* doc, const char* password)
	{
		return fz_authenticate_password(ctx, doc, password);
	}

	DLL_PUBLIC int CheckIfPasswordNeeded(fz_context* ctx, fz_document* doc)
	{
		return fz_needs_password(ctx, doc);
	}

	DLL_PUBLIC void ResetOutput(int stdoutFD, int stderrFD)
	{
		fprintf(stdout, "\n");
		fprintf(stderr, "\n");

		fflush(stdout);
		fflush(stderr);

		dup2(stdoutFD, fileno(stdout));
		dup2(stderrFD, fileno(stderr));

		fflush(stdout);
		fflush(stderr);
	}

	DLL_PUBLIC void WriteToFileDescriptor(int fileDescriptor, const char* text, int length)
	{
		while (length > 0)
		{
			int written = write(fileDescriptor, text, length);

			text += written;

			length -= written;
		}

	}

#if defined _WIN32
	void RedirectToPipe(const char* pipeName, int fd)
	{
		HANDLE hPipe = CreateNamedPipe(TEXT(pipeName), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 1, 1024 * 16, 1024 * 16, NMPWAIT_WAIT_FOREVER, NULL);

		ConnectNamedPipe(hPipe, NULL);

		int newFd = _open_osfhandle((intptr_t)hPipe, _O_WRONLY | _O_TEXT);
		dup2(newFd, fd);
	}
#else
	void RedirectToPipe(const char* pipeName, int fd)
	{
		struct sockaddr_un local;

		int sockFd = socket(AF_UNIX, SOCK_STREAM, 0);

		local.sun_family = AF_UNIX;
		strcpy(local.sun_path, pipeName);
		unlink(pipeName);

#if defined __APPLE__
		int len = strlen(local.sun_path) + sizeof(local.sun_family) + 1;
#elif defined __linux__
		int len = strlen(local.sun_path) + sizeof(local.sun_family);
#endif

		bind(sockFd, (struct sockaddr*)&local, len);

		listen(sockFd, INT32_MAX);

		int newFd = accept(sockFd, NULL, NULL);

		dup2(newFd, fd);
	}
#endif

	DLL_PUBLIC void RedirectOutput(int* stdoutFD, int* stderrFD, const char* stdoutPipe, const char* stderrPipe)
	{
		fflush(stdout);
		fflush(stderr);

		*stdoutFD = dup(fileno(stdout));
		/*FILE* stdoutFile = fopen("stdout.txt", "w");
		dup2(fileno(stdoutFile), fileno(stdout));*/

		RedirectToPipe(stdoutPipe, fileno(stdout));
		setvbuf(stdout, NULL, _IONBF, 0);

		*stderrFD = dup(fileno(stderr));
		/*FILE* stderrFile = fopen("stderr.txt", "w");
		dup2(fileno(stderrFile), fileno(stderr));*/
		RedirectToPipe(stderrPipe, fileno(stderr));
		setvbuf(stderr, NULL, _IONBF, 0);

		fflush(stdout);
		fflush(stderr);
	}


	DLL_PUBLIC int GetStructuredTextChar(fz_stext_char* character, int* out_c, int* out_color, float* out_origin_x, float* out_origin_y, float* out_size, float* out_ll_x, float* out_ll_y, float* out_ul_x, float* out_ul_y, float* out_ur_x, float* out_ur_y, float* out_lr_x, float* out_lr_y)
	{
		*out_c = character->c;

		*out_color = character->color;

		*out_origin_x = character->origin.x;
		*out_origin_y = character->origin.y;

		*out_size = character->size;

		*out_ll_x = character->quad.ll.x;
		*out_ll_y = character->quad.ll.y;

		*out_ul_x = character->quad.ul.x;
		*out_ul_y = character->quad.ul.y;

		*out_ur_x = character->quad.ur.x;
		*out_ur_y = character->quad.ur.y;

		*out_lr_x = character->quad.lr.x;
		*out_lr_y = character->quad.lr.y;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int GetStructuredTextChars(fz_stext_line* line, fz_stext_char** out_chars)
	{
		int count = 0;

		fz_stext_char* curr_char = line->first_char;

		while (curr_char != nullptr)
		{
			out_chars[count] = curr_char;
			count++;
			curr_char = curr_char->next;
		}

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int GetStructuredTextLine(fz_stext_line* line, int* out_wmode, float* out_x0, float* out_y0, float* out_x1, float* out_y1, float* out_x, float* out_y, int* out_char_count)
	{
		*out_wmode = line->wmode;

		*out_x0 = line->bbox.x0;
		*out_y0 = line->bbox.y0;
		*out_x1 = line->bbox.x1;
		*out_y1 = line->bbox.y1;

		*out_x = line->dir.x;
		*out_y = line->dir.y;

		int count = 0;

		fz_stext_char* curr_char = line->first_char;

		while (curr_char != nullptr)
		{
			count++;
			curr_char = curr_char->next;
		}

		*out_char_count = count;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int GetStructuredTextLines(fz_stext_block* block, fz_stext_line** out_lines)
	{
		int count = 0;

		fz_stext_line* curr_line = block->u.t.first_line;

		while (curr_line != nullptr)
		{
			out_lines[count] = curr_line;
			count++;
			curr_line = curr_line->next;
		}

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int GetStructuredTextBlock(fz_stext_block* block, int* out_type, float* out_x0, float* out_y0, float* out_x1, float* out_y1, int* out_line_count)
	{
		*out_type = block->type;

		*out_x0 = block->bbox.x0;
		*out_y0 = block->bbox.y0;
		*out_x1 = block->bbox.x1;
		*out_y1 = block->bbox.y1;

		if (block->type == FZ_STEXT_BLOCK_IMAGE)
		{
			*out_line_count = 0;
		} else if (block->type == FZ_STEXT_BLOCK_TEXT)
		{
			int count = 0;

			fz_stext_line* curr_line = block->u.t.first_line;

			while (curr_line != nullptr)
			{
				count++;
				curr_line = curr_line->next;
			}

			*out_line_count = count;
		}

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int GetStructuredTextBlocks(fz_stext_page* page, fz_stext_block** out_blocks)
	{
		fz_stext_block* curr_block = page->first_block;

		int count = 0;

		while (curr_block != nullptr)
		{
			out_blocks[count] = curr_block;
			count++;
			curr_block = curr_block->next;
		}

		return EXIT_SUCCESS;
	}

	typedef int (*progressCallback)(int progress);

	int progressFunction(fz_context* ctx, void* progress_arg, int progress)
	{
		return (*((progressCallback*)progress_arg))(progress);
	}

	DLL_PUBLIC int GetStructuredTextPageWithOCR(fz_context* ctx, fz_display_list* list, fz_stext_page** out_page, int* out_stext_block_count, float zoom, float x0, float y0, float x1, float y1, char* prefix, char* language, int callback(int))
	{
		if (prefix != NULL)
		{
			putenv(prefix);
		}

		fz_stext_page* page;
		fz_stext_options options;
		fz_device* device;
		fz_device* ocr_device;
		fz_rect bounds;
		fz_matrix ctm;

		ctm = fz_scale(zoom, zoom);

		bounds.x0 = x0;
		bounds.y0 = y0;
		bounds.x1 = x1;
		bounds.y1 = y1;

		fz_var(page);
		fz_var(device);

		fz_try(ctx)
		{
			page = fz_new_stext_page(ctx, fz_infinite_rect);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_CREATE_PAGE;
		}

		fz_try(ctx)
		{
			device = fz_new_stext_device(ctx, page, &options);

#if defined _WIN32 && (defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86))
			ocr_device = fz_new_ocr_device(ctx, device, ctm, bounds, true, language, NULL, NULL, NULL);
#else
			ocr_device = fz_new_ocr_device(ctx, device, ctm, bounds, true, language, NULL, progressFunction, &callback);
#endif

			fz_run_display_list(ctx, list, ocr_device, ctm, fz_infinite_rect, NULL);

			fz_close_device(ctx, ocr_device);
			fz_drop_device(ctx, ocr_device);
			ocr_device = NULL;

			fz_close_device(ctx, device);
			fz_drop_device(ctx, device);
			device = NULL;
		}
		fz_always(ctx)
		{
			fz_close_device(ctx, device);
			fz_drop_device(ctx, device);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_POPULATE_PAGE;
		}

		*out_page = page;

		int count = 0;

		fz_stext_block* curr_block = page->first_block;

		while (curr_block != nullptr)
		{
			count++;
			curr_block = curr_block->next;
		}

		*out_stext_block_count = count;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int GetStructuredTextPage(fz_context* ctx, fz_display_list* list, fz_stext_page** out_page, int* out_stext_block_count)
	{
		fz_stext_page* page;
		fz_stext_options options;
		fz_device* device;

		fz_try(ctx)
		{
			page = fz_new_stext_page(ctx, fz_infinite_rect);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_CREATE_PAGE;
		}

		fz_try(ctx)
		{
			device = fz_new_stext_device(ctx, page, &options);
			fz_run_display_list(ctx, list, device, fz_identity, fz_infinite_rect, NULL);
			fz_close_device(ctx, device);
		}
		fz_always(ctx)
		{
			fz_drop_device(ctx, device);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_POPULATE_PAGE;
		}

		*out_page = page;

		int count = 0;

		fz_stext_block* curr_block = page->first_block;

		while (curr_block != nullptr)
		{
			count++;
			curr_block = curr_block->next;
		}

		*out_stext_block_count = count;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int DisposeStructuredTextPage(fz_context* ctx, fz_stext_page* page)
	{
		fz_drop_stext_page(ctx, page);
		return EXIT_SUCCESS;
	}


	DLL_PUBLIC int FinalizeDocumentWriter(fz_context* ctx, fz_document_writer* writ)
	{
		fz_try(ctx)
		{
			fz_close_document_writer(ctx, writ);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_CLOSE_DOCUMENT;
		}

		fz_drop_document_writer(ctx, writ);

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int WriteSubDisplayListAsPage(fz_context* ctx, fz_display_list* list, float x0, float y0, float x1, float y1, float zoom, fz_document_writer* writ)
	{
		fz_device* dev;
		fz_rect rect;
		fz_matrix ctm;

		ctm = fz_concat(fz_translate(-x0, -y0), fz_scale(zoom, zoom));

		rect.x0 = x0;
		rect.y0 = y0;
		rect.x1 = x1;
		rect.y1 = y1;

		rect = fz_transform_rect(rect, ctm);

		fz_var(dev);

		fz_try(ctx)
		{
			dev = fz_begin_page(ctx, writ, rect);
			fz_run_display_list(ctx, list, dev, ctm, fz_infinite_rect, NULL);
			fz_end_page(ctx, writ);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_RENDER;
		}

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int CreateDocumentWriter(fz_context* ctx, const char* file_name, int format, const fz_document_writer** out_document_writer)
	{
		fz_document_writer* writ;

		fz_try(ctx)
		{
			switch (format)
			{
			case OUT_DOC_PDF:
				writ = fz_new_document_writer(ctx, file_name, "pdf", NULL);
				break;
			case OUT_DOC_SVG:
				writ = fz_new_document_writer(ctx, file_name, "svg", NULL);
				break;
			case OUT_DOC_CBZ:
				writ = fz_new_document_writer(ctx, file_name, "cbz", NULL);
				break;
			case OUT_DOC_DOCX:
				writ = fz_new_document_writer(ctx, file_name, "docx", NULL);
				break;
			case OUT_DOC_ODT:
				writ = fz_new_document_writer(ctx, file_name, "odt", NULL);
				break;
			case OUT_DOC_HTML:
				writ = fz_new_document_writer(ctx, file_name, "html", NULL);
				break;
			case OUT_DOC_XHTML:
				writ = fz_new_document_writer(ctx, file_name, "xhtml", NULL);
				break;
			}
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_CREATE_WRITER;
		}

		*out_document_writer = writ;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int WriteImage(fz_context* ctx, fz_display_list* list, float x0, float y0, float x1, float y1, float zoom, int colorFormat, int output_format, const fz_buffer** out_buffer, const unsigned char** out_data, uint64_t* out_length)
	{
		fz_matrix ctm;
		fz_pixmap* pix;
		fz_output* out;
		fz_buffer* buf;
		fz_rect rect;
		int alpha;
		fz_colorspace* cs;

		ctm = fz_scale(zoom, zoom);

		rect.x0 = x0;
		rect.y0 = y0;
		rect.x1 = x1;
		rect.y1 = y1;

		fz_var(out);
		fz_var(buf);

		fz_try(ctx)
		{
			buf = fz_new_buffer(ctx, 1024);
			out = fz_new_output_with_buffer(ctx, buf);
		}
		fz_catch(ctx)
		{
			fz_drop_buffer(ctx, buf);
			return ERR_CANNOT_CREATE_BUFFER;
		}

		switch (colorFormat)
		{
		case COLOR_RGB:
			cs = fz_device_rgb(ctx);
			alpha = 0;
			break;
		case COLOR_RGBA:
			cs = fz_device_rgb(ctx);
			alpha = 1;
			break;
		case COLOR_BGR:
			cs = fz_device_bgr(ctx);
			alpha = 0;
			break;
		case COLOR_BGRA:
			cs = fz_device_bgr(ctx);
			alpha = 1;
			break;
		}


		//Render page to an RGB/RGBA pixmap.
		fz_try(ctx)
		{
			pix = new_pixmap_from_display_list_with_separations_bbox(ctx, list, rect, ctm, cs, NULL, alpha);
		}
		fz_catch(ctx)
		{
			fz_drop_output(ctx, out);
			fz_drop_buffer(ctx, buf);
			return ERR_CANNOT_RENDER;
		}

		//Write the rendered pixmap to the output buffer in the specified format.
		fz_try(ctx)
		{
			switch (output_format)
			{
			case OUT_PNM:
				fz_write_pixmap_as_pnm(ctx, out, pix);
				break;
			case OUT_PAM:
				fz_write_pixmap_as_pam(ctx, out, pix);
				break;
			case OUT_PNG:
				fz_write_pixmap_as_png(ctx, out, pix);
				break;
			case OUT_PSD:
				fz_write_pixmap_as_psd(ctx, out, pix);
				break;
			}
		}
		fz_catch(ctx)
		{
			fz_drop_output(ctx, out);
			fz_drop_buffer(ctx, buf);
			fz_drop_pixmap(ctx, pix);
			return ERR_CANNOT_SAVE;
		}

		fz_close_output(ctx, out);
		fz_drop_output(ctx, out);
		fz_drop_pixmap(ctx, pix);

		*out_buffer = buf;
		*out_data = buf->data;
		*out_length = buf->len;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int DisposeBuffer(fz_context* ctx, fz_buffer* buf)
	{
		fz_drop_buffer(ctx, buf);
		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int SaveImage(fz_context* ctx, fz_display_list* list, float x0, float y0, float x1, float y1, float zoom, int colorFormat, const char* file_name, int output_format)
	{
		fz_matrix ctm;
		fz_pixmap* pix;
		fz_rect rect;
		int alpha;
		fz_colorspace* cs;

		switch (colorFormat)
		{
		case COLOR_RGB:
			cs = fz_device_rgb(ctx);
			alpha = 0;
			break;
		case COLOR_RGBA:
			cs = fz_device_rgb(ctx);
			alpha = 1;
			break;
		case COLOR_BGR:
			cs = fz_device_bgr(ctx);
			alpha = 0;
			break;
		case COLOR_BGRA:
			cs = fz_device_bgr(ctx);
			alpha = 1;
			break;
		}


		ctm = fz_scale(zoom, zoom);

		rect.x0 = x0;
		rect.y0 = y0;
		rect.x1 = x1;
		rect.y1 = y1;

		//Render page to an RGB/RGBA pixmap.
		fz_try(ctx)
		{
			pix = new_pixmap_from_display_list_with_separations_bbox(ctx, list, rect, ctm, cs, NULL, alpha);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_RENDER;
		}

		//Save the rendered pixmap to the output file in the specified format.
		fz_try(ctx)
		{
			switch (output_format)
			{
			case OUT_PNM:
				fz_save_pixmap_as_pnm(ctx, pix, file_name);
				break;
			case OUT_PAM:
				fz_save_pixmap_as_pam(ctx, pix, file_name);
				break;
			case OUT_PNG:
				fz_save_pixmap_as_png(ctx, pix, file_name);
				break;
			case OUT_PSD:
				fz_save_pixmap_as_psd(ctx, pix, file_name);
				break;
			}
		}
		fz_catch(ctx)
		{
			fz_drop_pixmap(ctx, pix);
			return ERR_CANNOT_SAVE;
		}

		fz_drop_pixmap(ctx, pix);

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int CloneContext(fz_context* ctx, int count, fz_context** out_contexts)
	{
		for (int i = 0; i < count; i++)
		{
			fz_try(ctx)
			{
				fz_context* curr_ctx = fz_clone_context(ctx);
				fz_var(curr_ctx);

				out_contexts[i] = curr_ctx;

				if (!curr_ctx)
				{
					for (int j = 0; j < i; j++)
					{
						fz_drop_context(out_contexts[j]);
					}
					return ERR_CANNOT_CLONE_CONTEXT;
				}
			}
			fz_catch(ctx)
			{
				for (int j = 0; j < i; j++)
				{
					fz_drop_context(out_contexts[j]);
				}
				return ERR_CANNOT_CLONE_CONTEXT;
			}
		}

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int RenderSubDisplayList(fz_context* ctx, fz_display_list* list, float x0, float y0, float x1, float y1, float zoom, int colorFormat, unsigned char* pixel_storage, fz_cookie* cookie)
	{
		if (cookie != NULL && cookie->abort)
		{
			return EXIT_SUCCESS;
		}

		fz_matrix ctm;
		fz_pixmap* pix;
		fz_rect rect;
		int alpha;
		fz_colorspace* cs;
		switch (colorFormat)
		{
		case COLOR_RGB:
			cs = fz_device_rgb(ctx);
			alpha = 0;
			break;
		case COLOR_RGBA:
			cs = fz_device_rgb(ctx);
			alpha = 1;
			break;
		case COLOR_BGR:
			cs = fz_device_bgr(ctx);
			alpha = 0;
			break;
		case COLOR_BGRA:
			cs = fz_device_bgr(ctx);
			alpha = 1;
			break;
		}

		ctm = fz_scale(zoom, zoom);

		rect.x0 = x0;
		rect.y0 = y0;
		rect.x1 = x1;
		rect.y1 = y1;

		//Render page to an RGB/RGBA pixmap.
		fz_try(ctx)
		{
			pix = new_pixmap_from_display_list_with_separations_bbox_and_data(ctx, list, rect, ctm, cs, NULL, alpha, pixel_storage, cookie);
		}
		fz_catch(ctx)
		{

			return ERR_CANNOT_RENDER;
		}

		fz_drop_pixmap(ctx, pix);

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int GetDisplayList(fz_context* ctx, fz_page* page, int annotations, fz_display_list** out_display_list, float* out_x0, float* out_y0, float* out_x1, float* out_y1)
	{
		fz_display_list* list;
		fz_rect bounds;
		fz_device* bbox;

		fz_try(ctx)
		{
			if (annotations == 1)
			{
				list = fz_new_display_list_from_page(ctx, page);
			} else
			{
				list = fz_new_display_list_from_page_contents(ctx, page);
			}
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_RENDER;
		}

		fz_var(bbox);

		fz_try(ctx)
		{
			bbox = fz_new_bbox_device(ctx, &bounds);
			fz_run_display_list(ctx, list, bbox, fz_identity, fz_infinite_rect, NULL);
			fz_close_device(ctx, bbox);
		}
		fz_always(ctx)
		{
			fz_drop_device(ctx, bbox);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_COMPUTE_BOUNDS;
		}

		*out_display_list = list;

		*out_x0 = bounds.x0;
		*out_y0 = bounds.y0;
		*out_x1 = bounds.x1;
		*out_y1 = bounds.y1;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int DisposeDisplayList(fz_context* ctx, fz_display_list* list)
	{
		fz_drop_display_list(ctx, list);
		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int LoadPage(fz_context* ctx, fz_document* doc, int page_number, const fz_page** out_page, float* out_x, float* out_y, float* out_w, float* out_h)
	{
		fz_page* page;
		fz_rect bounds;

		fz_try(ctx)
		{
			page = fz_load_page(ctx, doc, page_number);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_LOAD_PAGE;
		}

		fz_try(ctx)
		{
			bounds = fz_bound_page(ctx, page);
		}
		fz_catch(ctx)
		{
			fz_drop_page(ctx, page);
			return ERR_CANNOT_COMPUTE_BOUNDS;
		}

		*out_x = bounds.x0;
		*out_y = bounds.y0;
		*out_w = bounds.x1 - bounds.x0;
		*out_h = bounds.y1 - bounds.y0;

		*out_page = page;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int DisposePage(fz_context* ctx, fz_page* page)
	{
		fz_drop_page(ctx, page);
		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int CreateDocumentFromFile(fz_context* ctx, const char* file_name, int get_image_resolution, const fz_document** out_doc, int* out_page_count, float* out_image_xres, float* out_image_yres)
	{
		if (get_image_resolution != 0)
		{
			fz_image* img;

			fz_try(ctx)
			{
				img = fz_new_image_from_file(ctx, file_name);

				if (img != nullptr)
				{
					*out_image_xres = img->xres;
					*out_image_yres = img->yres;
				} else
				{
					*out_image_xres = -1;
					*out_image_yres = -1;
				}

				fz_drop_image(ctx, img);
			}
			fz_catch(ctx)
			{
				*out_image_xres = -1;
				*out_image_yres = -1;
			}
		} else
		{
			*out_image_xres = -1;
			*out_image_yres = -1;
		}

		fz_document* doc;

		//Open the document.
		fz_try(ctx)
		{
			doc = fz_open_document(ctx, file_name);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_OPEN_FILE;
		}

		//Count the number of pages.
		fz_try(ctx)
		{
			*out_page_count = fz_count_pages(ctx, doc);
		}
		fz_catch(ctx)
		{
			fz_drop_document(ctx, doc);
			return ERR_CANNOT_COUNT_PAGES;
		}

		*out_doc = doc;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int CreateDocumentFromStream(fz_context* ctx, const unsigned char* data, const uint64_t data_length, const char* file_type, int get_image_resolution, const fz_document** out_doc, const fz_stream** out_str, int* out_page_count, float* out_image_xres, float* out_image_yres)
	{

		fz_stream* str;
		fz_document* doc;

		fz_try(ctx)
		{
			str = fz_open_memory(ctx, data, data_length);
		}
		fz_catch(ctx)
		{
			return ERR_CANNOT_OPEN_STREAM;
		}

		if (get_image_resolution != 0)
		{
			fz_image* img;
			fz_buffer* img_buf;

			int bufferCreated = 0;

			fz_try(ctx)
			{
				img_buf = fz_new_buffer_from_shared_data(ctx, data, data_length);
				bufferCreated = 1;
			}
			fz_catch(ctx)
			{
				bufferCreated = 0;
			}

			if (bufferCreated == 1)
			{
				fz_try(ctx)
				{
					img = fz_new_image_from_buffer(ctx, img_buf);

					if (img != nullptr)
					{
						*out_image_xres = img->xres;
						*out_image_yres = img->yres;
					} else
					{
						*out_image_xres = -1;
						*out_image_yres = -1;
					}

					fz_drop_image(ctx, img);
				}
				fz_catch(ctx)
				{
					*out_image_xres = -1;
					*out_image_yres = -1;
				}

				fz_drop_buffer(ctx, img_buf);
			} else
			{
				*out_image_xres = -1;
				*out_image_yres = -1;
			}
		} else
		{
			*out_image_xres = -1;
			*out_image_yres = -1;
		}

		//Open the document.
		fz_try(ctx)
		{
			doc = fz_open_document_with_stream(ctx, file_type, str);
		}
		fz_catch(ctx)
		{

			fz_drop_stream(ctx, str);
			return ERR_CANNOT_OPEN_FILE;
		}

		//Count the number of pages.
		fz_try(ctx)
		{
			*out_page_count = fz_count_pages(ctx, doc);
		}
		fz_catch(ctx)
		{

			fz_drop_document(ctx, doc);
			fz_drop_stream(ctx, str);
			return ERR_CANNOT_COUNT_PAGES;
		}

		*out_str = str;
		*out_doc = doc;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int DisposeStream(fz_context* ctx, fz_stream* str)
	{
		fz_drop_stream(ctx, str);
		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int DisposeDocument(fz_context* ctx, fz_document* doc)
	{
		fz_drop_document(ctx, doc);
		return EXIT_SUCCESS;
	}

	DLL_PUBLIC uint64_t GetCurrentStoreSize(const fz_context* ctx)
	{
		return ctx->store->size;
	}

	DLL_PUBLIC uint64_t GetMaxStoreSize(const fz_context* ctx)
	{
		return ctx->store->max;
	}

	DLL_PUBLIC int ShrinkStore(fz_context* ctx, unsigned int perc)
	{
		return fz_shrink_store(ctx, perc);
	}

	DLL_PUBLIC void EmptyStore(fz_context* ctx)
	{
		fz_empty_store(ctx);
	}

	DLL_PUBLIC int CreateContext(uint64_t store_size, const fz_context** out_ctx)
	{
		fz_context* ctx;
		fz_locks_context locks;

		//Create lock objects necessary for multithreaded context operations.
		locks.user = &global_mutex;
		locks.lock = lock_mutex;
		locks.unlock = unlock_mutex;

		lock_mutex(locks.user, 0);
		unlock_mutex(locks.user, 0);

		//Create a context to hold the exception stack and various caches.
		ctx = fz_new_context(NULL, &locks, store_size);
		if (!ctx)
		{
			return ERR_CANNOT_CREATE_CONTEXT;
		}

		fz_var(ctx);

		//Register the default file types to handle.
		fz_try(ctx)
		{
			fz_register_document_handlers(ctx);
		}
		fz_catch(ctx)
		{
			fz_drop_context(ctx);

			return ERR_CANNOT_REGISTER_HANDLERS;
		}

		*out_ctx = ctx;

		return EXIT_SUCCESS;
	}

	DLL_PUBLIC int DisposeContext(fz_context* ctx)
	{
		fz_drop_context(ctx);
		return EXIT_SUCCESS;
	}

	DLL_PUBLIC void SetLayoutParameters(fz_context* ctx, fz_document* doc, float w, float h, float em)
	{
		fz_layout_document(ctx, doc, w, h, em);
		(void)fz_count_pages(ctx, doc);
	}

	DLL_PUBLIC void InstallLoadSystemFonts(fz_context* ctx)
	{
		return fz_install_load_system_font_funcs(ctx,
			load_droid_font,
			load_droid_cjk_font,
			load_droid_fallback_font);
	}
}
