#include "testpango.h"
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>

#include <assert.h>

#include <stdint.h>

#include <iconv.h>

#include <cairo-win32.h>
#include <pango/pangoft2.h>
#include <pango/pangocairo.h>

#include <fontconfig/fontconfig.h>

#include <list>
#include <vector>

/**********************************************************************************************
 *
 **********************************************************************************************/
#define	IMAGE_WIDTH		(800)
#define	IMAGE_HEIGHT	(280)

/**********************************************************************************************
 *
 **********************************************************************************************/
static void init_fonts();
static PangoFontDescription* get_font_desc(const char *family, float size);
static void draw_text_on_surface(cairo_surface_t *destine_surface, const wchar_t *wchar_string, const char *font_family, float font_size, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/*+-------------------------------------------------------------------------+
  |  FILE: sprintf.c                                                        |
  |  Version: 0.1                                                           |
  |                                                                         |
  |  Copyright (c) 2003 Chun Joon Sung                                      |
  |  Email: <a href="mailto:chunjoonsung@hanmail.net">chunjoonsung@hanmail.net</a>                                        |
  +-------------------------------------------------------------------------+*/
WCHAR *itoa( WCHAR *a, int i)
{
	int sign=0;
	int temp=0;
	WCHAR buf[16];
	WCHAR *ptr;

	ptr = buf;

	/* zero then return */
	if( i )
	{
		/* make string in reverse form */
		if( i < 0 )
			{ i = ~i + 1; sign++; }
		while( i )
			{ *ptr++ = (i % 10) + '0'; i = i / 10; }
		if( sign )
			*ptr++ = '-';
		*ptr = '\0';

		/* copy reverse order */
		for( i=0; i < wcslen(buf); i++ )
			*a++ = buf[wcslen(buf)-i-1];
	}	
	else
		*a++ = '0';

	return a;
}

WCHAR *xtoa( WCHAR *a, unsigned int x, int opt)
{
	int i;
	int sign=0;
	int temp=0;
	WCHAR buf[16];
	WCHAR *ptr;

	ptr = buf;

	/* zero then return */
	if( x )
	{
		/* make string in reverse form */
		while( x )
			{ *ptr++ = (x&0x0f)<10 ? (x&0x0f)+'0' : (x&0x0f)-10+opt; x>>= 4; }

		*ptr = '\0';

		/* copy reverse order */
		for( i=0; i < wcslen(buf); i++ )
			*a++ = buf[wcslen(buf)-i-1];
	}
	else	
		*a++ = '0';

	return a;
}

long _wsprintf(WCHAR* buf, WCHAR* format, long arg)
{
	int cont_flag;
	int value;
	int quit;
	WCHAR *start=buf;
	long *argp=(long *)&arg;
	WCHAR *p;

  while( *format )
	{
		if (*format == '%')
		{
			format++;				/* skip '%' */

			if( *format == '%' )	/* '%' 문자가 연속 두번 있는 경우 */
			{
				*buf++ = *format++;
				continue;
			}

			switch( *format )
			{
				case 'c' :
					*buf++ = *(char *)argp++;
					break;

				case 'd' :				
					buf = itoa(buf,*(int *)argp++);
					break;

				case 'x' : 
					buf = xtoa(buf,*(unsigned int *)argp++,'a');
					break;

				case 'X' : 
					buf = xtoa(buf,*(unsigned int *)argp++,'A');
					break;

				case 's' :
				case '1' :
					p=*(WCHAR **)argp++;
					while(*p) 
							*buf++ = *p++;
					break;

				default :
					*buf++ = *format; /* % 뒤에 엉뚱한 문자인 경우 */
					break;
			}

			format++;
    }
		else if (*format == '\\')
		{
			format++;				/* skip '%' */

			if( *format == '\\' )	/* '%' 문자가 연속 두번 있는 경우 */
			{
				*buf++ = *format++;
				continue;
			}

			switch( *format )
			{
				case 'n' :
					*buf++ = 0x0A;
					break;

				default :
					*buf++ = *format; /* % 뒤에 엉뚱한 문자인 경우 */
					break;
			}

			format++;
    }
		else
		{
			*buf++ = *format++;
		}
	}
  *buf = '\0';

  return(buf-start);
}


/**********************************************************************************************
 *
 **********************************************************************************************/
class PangoAttributes
{
public:
	QString text;
	int fontFamilyIndex;
	int fontSize;
	int layoutX, layoutY;
	int layoutWidth, layoutHeight;
	bool drawLayout;
	int indent;
	int spacing;
	int wrapModeIndex;
	int ellipsizeModeIndex;
	bool justify;
	bool auto_dir;
	int alignmentIndex;
	bool singlParagraphMode;
	uint8_t fr, fg, fb;
	uint8_t br, bg, bb;

public:
	static PangoAttributes* getInstance()
	{
		static PangoAttributes* s_instance = NULL;
		if(!s_instance)
		{
			s_instance = new PangoAttributes();
			assert(s_instance);
		}
		return s_instance;
	}

private:
	PangoAttributes()
		: text("Sample Text")
		, fontFamilyIndex(0)
		, fontSize(20)
		, layoutX(0)
		, layoutY(0)
		, layoutWidth(800)
		, layoutHeight(480)
		, drawLayout(false)
		, indent(0)
		, spacing(0)
		, wrapModeIndex(0)
		, ellipsizeModeIndex(0)
		, justify(false)
		, auto_dir(false)
		, alignmentIndex(0)
		, singlParagraphMode(false)
		, fr(0)
		, fg(0)
		, fb(0)
		, br(255)
		, bg(255)
		, bb(255)
	{
	}
};

/**********************************************************************************************
 *
 **********************************************************************************************/
static PangoAttribute g_pangoAttributes;
static PangoCairoFontMap* s_fontmap = NULL;
static PangoContext *s_pango_context = NULL;

/**********************************************************************************************
 *
 **********************************************************************************************/
typedef std::list< QString > FontFamilyList;
static FontFamilyList g_fontFamilyList;

#define	ENUM_TO_PAIR(e)	std::make_pair(e, #e)

typedef std::vector< std::pair< PangoWrapMode, QString > > PangoWrapModeList;
static PangoWrapModeList g_pangoWrapModeList;

typedef std::vector< std::pair< PangoEllipsizeMode, QString> > PangoEllipsizeModeList;
static PangoEllipsizeModeList g_pangoEllipsizeModeList;

typedef std::vector< std::pair< PangoAlignment, QString> > PangoAlignmentList;
static PangoAlignmentList g_pangoAlignmentList;

/**********************************************************************************************
 *
 **********************************************************************************************/
TestPango::TestPango(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
	, m_image(IMAGE_WIDTH, IMAGE_HEIGHT, QImage::Format_ARGB32)
{
	init_fonts();

	ui.setupUi(this);
	connect(ui.button_apply, SIGNAL(clicked()), this, SLOT(onApply()));
	connect(ui.buttonOpenUTF8File, SIGNAL(clicked()), this, SLOT(onOpenUTF8File()));

	idx = 0;
	isSpellerEnter = false;
	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(appendText1()));
	connect(ui.pushButton_2, SIGNAL(clicked()), this, SLOT(appendText2()));
	connect(ui.pushButton_3, SIGNAL(clicked()), this, SLOT(appendText3()));
	connect(ui.pushButton_4, SIGNAL(clicked()), this, SLOT(appendText4()));
	connect(ui.pushButton_5, SIGNAL(clicked()), this, SLOT(appendText5()));
	connect(ui.pushButton_6, SIGNAL(clicked()), this, SLOT(appendText6()));
	connect(ui.pushButton_7, SIGNAL(clicked()), this, SLOT(appendText7()));
	connect(ui.pushButton_8, SIGNAL(clicked()), this, SLOT(appendText8()));
	connect(ui.pushButton_9, SIGNAL(clicked()), this, SLOT(appendText9()));
	connect(ui.pushButton_10, SIGNAL(clicked()), this, SLOT(appendText10()));

	connect(ui.pushButton_11, SIGNAL(clicked()), this, SLOT(appendText11()));
	connect(ui.pushButton_12, SIGNAL(clicked()), this, SLOT(appendText12()));
	connect(ui.pushButton_13, SIGNAL(clicked()), this, SLOT(appendText13()));
	connect(ui.pushButton_14, SIGNAL(clicked()), this, SLOT(appendText14()));
	connect(ui.pushButton_15, SIGNAL(clicked()), this, SLOT(appendText15()));
	connect(ui.pushButton_16, SIGNAL(clicked()), this, SLOT(appendText16()));
	connect(ui.pushButton_17, SIGNAL(clicked()), this, SLOT(appendText17()));
	connect(ui.pushButton_18, SIGNAL(clicked()), this, SLOT(appendText18()));
	connect(ui.pushButton_19, SIGNAL(clicked()), this, SLOT(appendText19()));
	connect(ui.pushButton_20, SIGNAL(clicked()), this, SLOT(appendText20()));

	connect(ui.pushButton_21, SIGNAL(clicked()), this, SLOT(appendText21()));
	connect(ui.pushButton_22, SIGNAL(clicked()), this, SLOT(appendText22()));
	connect(ui.pushButton_23, SIGNAL(clicked()), this, SLOT(appendText23()));
	connect(ui.pushButton_24, SIGNAL(clicked()), this, SLOT(appendText24()));
	connect(ui.pushButton_25, SIGNAL(clicked()), this, SLOT(appendText25()));
	connect(ui.pushButton_26, SIGNAL(clicked()), this, SLOT(appendText26()));
	connect(ui.pushButton_27, SIGNAL(clicked()), this, SLOT(appendText27()));
	connect(ui.pushButton_28, SIGNAL(clicked()), this, SLOT(appendText28()));
	connect(ui.pushButton_29, SIGNAL(clicked()), this, SLOT(appendText29()));
	connect(ui.pushButton_30, SIGNAL(clicked()), this, SLOT(appendText30()));

	connect(ui.pushButton_31, SIGNAL(clicked()), this, SLOT(appendText31()));
	connect(ui.pushButton_32, SIGNAL(clicked()), this, SLOT(appendText32()));
	connect(ui.pushButton_33, SIGNAL(clicked()), this, SLOT(appendText33()));
	connect(ui.pushButton_34, SIGNAL(clicked()), this, SLOT(appendText34()));
	connect(ui.pushButton_35, SIGNAL(clicked()), this, SLOT(appendText35()));
	connect(ui.pushButton_36, SIGNAL(clicked()), this, SLOT(appendText36()));
	connect(ui.pushButton_37, SIGNAL(clicked()), this, SLOT(appendText37()));
	connect(ui.pushButton_38, SIGNAL(clicked()), this, SLOT(appendText38()));
	connect(ui.pushButton_39, SIGNAL(clicked()), this, SLOT(appendText39()));
	connect(ui.pushButton_40, SIGNAL(clicked()), this, SLOT(appendText40()));
	connect(ui.pushButton_41, SIGNAL(clicked()), this, SLOT(appendText41()));
	connect(ui.pushButton_42, SIGNAL(clicked()), this, SLOT(appendText42()));
	connect(ui.pushButton_43, SIGNAL(clicked()), this, SLOT(appendText43()));
	connect(ui.pushButton_44, SIGNAL(clicked()), this, SLOT(appendText44()));
	connect(ui.pushButton_45, SIGNAL(clicked()), this, SLOT(appendText45()));
	connect(ui.pushButton_46, SIGNAL(clicked()), this, SLOT(appendText46()));
	connect(ui.pushButton_47, SIGNAL(clicked()), this, SLOT(appendText47()));
	connect(ui.pushButton_48, SIGNAL(clicked()), this, SLOT(appendText48()));
	connect(ui.pushButton_49, SIGNAL(clicked()), this, SLOT(appendText49()));
	connect(ui.pushButton_50, SIGNAL(clicked()), this, SLOT(appendText50()));
	connect(ui.pushButton_51, SIGNAL(clicked()), this, SLOT(appendText51()));

	init_font_family();
	init_pango_enums();

	update_attributes_to_ui();

	update_text();
}

TestPango::~TestPango()
{

}

void TestPango::paintEvent(QPaintEvent * event)
{
	drawImage();

	QMainWindow::paintEvent(event);
}

void TestPango::drawImage()
{
	ui.labelPixmap->setPixmap(QPixmap::fromImage(m_image));
}

void TestPango::update_attributes_to_ui()
{
	PangoAttributes* attributes = PangoAttributes::getInstance();

	ui.textEdit->setText(attributes->text);

	ui.lineEditX->setText(QString::number(attributes->layoutX));
	ui.lineEditY->setText(QString::number(attributes->layoutY));

	ui.comboBoxFontFamily->setCurrentIndex(attributes->fontFamilyIndex);
	
	ui.lineEditSize->setText(QString::number(attributes->fontSize));
	
	ui.lineEditWidth->setText(QString::number(attributes->layoutWidth));
	ui.lineEditHeight->setText(QString::number(attributes->layoutHeight));
	ui.checkBoxDrawLayout->setCheckState(attributes->drawLayout ? Qt::Checked : Qt::Unchecked);

	ui.comboBoxWrap->setCurrentIndex(attributes->wrapModeIndex);
	ui.comboBoxEllipse->setCurrentIndex(attributes->ellipsizeModeIndex);

	ui.lineEditIndent->setText(QString::number(attributes->indent));
	ui.lineEditSpacing->setText(QString::number(attributes->spacing));
	ui.checkBoxJustify->setCheckState(attributes->justify ? Qt::Checked : Qt::Unchecked);
	ui.checkBoxAutoDir->setCheckState(attributes->auto_dir ? Qt::Checked : Qt::Unchecked);

	ui.comboBoxAlignment->setCurrentIndex(attributes->alignmentIndex);

	ui.checkBoxSingleParagraphMode->setCheckState(attributes->singlParagraphMode ? Qt::Checked : Qt::Unchecked);

	ui.lineEditForeRed->setText(QString::number(attributes->fr));
	ui.lineEditForeGreen->setText(QString::number(attributes->fg));
	ui.lineEditForeBlue->setText(QString::number(attributes->fb));

	ui.lineEditBackRed->setText(QString::number(attributes->br));
	ui.lineEditBackGreen->setText(QString::number(attributes->bg));
	ui.lineEditBackBlue->setText(QString::number(attributes->bb));
}

void TestPango::update_attributes_from_ui()
{
	PangoAttributes* attributes = PangoAttributes::getInstance();

	if (isSpellerEnter) {
		attributes->text = str;
	} else {
		QString text = ui.textEdit->toPlainText();

		WCHAR format[1000] = {0, };
		wsprintf(format, L"%s", text.data());

		WCHAR dest[1000] = {0, };
		_wsprintf(dest, format, (long) ui.textParam->text().data());

		//	char mbs[1000] = {0, };
		//	wcstombs(mbs, dest, 1000);

		attributes->text = QString::fromWCharArray(dest);
	}

	//attributes->text = ui.textEdit->toPlainText();
	
	attributes->layoutX = ui.lineEditX->text().toInt();
	attributes->layoutY = ui.lineEditY->text().toInt();

	attributes->fontFamilyIndex = ui.comboBoxFontFamily->currentIndex();
	attributes->fontSize = ui.lineEditSize->text().toInt();

	attributes->layoutWidth = ui.lineEditWidth->text().toInt();
	attributes->layoutHeight = ui.lineEditHeight->text().toInt();
	attributes->drawLayout = ui.checkBoxDrawLayout->isChecked();

	attributes->wrapModeIndex = ui.comboBoxWrap->currentIndex();
	attributes->ellipsizeModeIndex = ui.comboBoxEllipse->currentIndex();

	attributes->indent = ui.lineEditIndent->text().toInt();
	attributes->spacing = ui.lineEditSpacing->text().toInt();
	attributes->justify = ui.checkBoxJustify->isChecked();
	attributes->auto_dir = ui.checkBoxAutoDir->isChecked();

	attributes->alignmentIndex = ui.comboBoxAlignment->currentIndex();

	attributes->singlParagraphMode = ui.checkBoxSingleParagraphMode->isChecked();

	attributes->fr = ui.lineEditForeRed->text().toInt();
	attributes->fg = ui.lineEditForeGreen->text().toInt();
	attributes->fb = ui.lineEditForeBlue->text().toInt();

	attributes->br = ui.lineEditBackRed->text().toInt();
	attributes->bg = ui.lineEditBackGreen->text().toInt();
	attributes->bb = ui.lineEditBackBlue->text().toInt();
}

void TestPango::onApply()
{
	isSpellerEnter = false;
	onApplyAttributes();
}

void TestPango::onApplSpeller()
{
	isSpellerEnter = true;
	onApplyAttributes();
}

void TestPango::onApplyAttributes()
{
	update_attributes_from_ui();
	update_text();
	drawImage();
	// ui.labelPixmap->repaint();
}

void TestPango::onOpenUTF8File()
{
	QString fileName = QFileDialog::getOpenFileName(this, QString("UTF8 File"), QString(), QString("Text Files (*.txt)"));
	if(fileName.size() > 0)
	{
		QFile file(fileName);
		if(file.open(QIODevice::ReadOnly))
		{
			QTextStream in(&file);

			QString text;
			while(!in.atEnd()) {
				QString line = in.readLine();
				text += line + '\n';
			}

			file.close();

			PangoAttributes* attributes = PangoAttributes::getInstance();

			attributes->text = text;
			ui.textEdit->setText(attributes->text);
		}
		else
		{
			QMessageBox::critical(this, "Error", "Failed to open file!");
		}
	}
}

void TestPango::update_text()
{
	PangoAttributes* attributes = PangoAttributes::getInstance();
	
	cairo_surface_t *surface_pixmap = cairo_image_surface_create_for_data(m_image.bits(), CAIRO_FORMAT_ARGB32, IMAGE_WIDTH, IMAGE_HEIGHT, m_image.bytesPerLine());

	cairo_t *cr_pixmap = cairo_create(surface_pixmap);
	cairo_set_source_rgb(cr_pixmap, attributes->br / 255.0, attributes->bg / 255.0, attributes->bb / 255.0);
	cairo_paint(cr_pixmap);
	cairo_destroy(cr_pixmap);

	cairo_t* cr_destine_surface = cairo_create(surface_pixmap);
	if(cr_destine_surface)
	{
		cairo_translate(cr_destine_surface, attributes->layoutX, attributes->layoutY);

		PangoLayout *pango_layout = pango_cairo_create_layout(cr_destine_surface);
		if(pango_layout)
		{
			pango_layout_set_text(pango_layout, attributes->text.toUtf8().data(), -1);

			pango_layout_set_width(pango_layout, attributes->layoutWidth > 0 ? (attributes->layoutWidth * PANGO_SCALE) : attributes->layoutWidth);
			pango_layout_set_height(pango_layout, (attributes->layoutHeight > 0) ? (attributes->layoutHeight * PANGO_SCALE) : attributes->layoutHeight);

			pango_layout_set_wrap(pango_layout, g_pangoWrapModeList[attributes->wrapModeIndex].first);

			pango_layout_set_ellipsize(pango_layout, g_pangoEllipsizeModeList[attributes->ellipsizeModeIndex].first);

			pango_layout_set_indent(pango_layout, attributes->indent > 0 ? (attributes->indent * PANGO_SCALE) : attributes->indent);
			pango_layout_set_spacing(pango_layout, attributes->spacing > 0 ? (attributes->spacing * PANGO_SCALE) : attributes->spacing);

			pango_layout_set_justify(pango_layout, attributes->justify);
			pango_layout_set_auto_dir(pango_layout, attributes->auto_dir);

			pango_layout_set_alignment(pango_layout, g_pangoAlignmentList[attributes->alignmentIndex].first);

			pango_layout_set_single_paragraph_mode(pango_layout, attributes->singlParagraphMode);

			QByteArray fontFamlyData = ui.comboBoxFontFamily->itemText(attributes->fontFamilyIndex).toAscii();
			PangoFontDescription *pango_desc = get_font_desc(fontFamlyData.data(), attributes->fontSize);
			if(pango_desc)
			{
				pango_layout_set_font_description(pango_layout, pango_desc);
				pango_font_description_free(pango_desc);
			}
	
			cairo_set_source_rgb(cr_destine_surface, attributes->fr/255.0, attributes->fg/255.0, attributes->fb/255.0);
			pango_cairo_update_layout(cr_destine_surface, pango_layout);
			pango_cairo_show_layout(cr_destine_surface, pango_layout);

			g_object_unref(pango_layout);
		}

		if(attributes->drawLayout && (attributes->layoutWidth > 0 || attributes->layoutHeight > 0))
		{
			cairo_set_source_rgb(cr_destine_surface, 1.0, 0.0, 0.0);
			cairo_set_line_width(cr_destine_surface, 1.0);
			cairo_set_line_join(cr_destine_surface, CAIRO_LINE_JOIN_MITER); 

			int rectWidth = IMAGE_WIDTH, rectHeight = IMAGE_HEIGHT;
			
			if(attributes->layoutWidth > 0)
			{
				rectWidth = attributes->layoutWidth;
			}

			if(attributes->layoutHeight > 0)
			{
				rectHeight = attributes->layoutHeight;
			}

			cairo_rectangle(cr_destine_surface, -0.5, -0.5, rectWidth + 1.0, rectHeight + 1.0);
			cairo_stroke(cr_destine_surface);    
		}

		cairo_destroy(cr_destine_surface);
	}
}

void TestPango::init_font_family ()
{
#if 0
	// Not working... ???
    PangoFontFamily ** families;
    int n_families;
    PangoFontMap * fontmap;

    fontmap = pango_cairo_font_map_get_default();
	// fontmap = (PangoFontMap*)s_fontmap;
    pango_font_map_list_families (fontmap, & families, & n_families);
    printf ("There are %d families\n", n_families);
    for (int i = 0; i < n_families; i++) {
        PangoFontFamily * family = families[i];
        const char * family_name;

        family_name = pango_font_family_get_name (family);
		g_fontFamilyList.push_back(QString(family_name));
    }
    g_free (families);
#else
	g_fontFamilyList.push_back(QString("Skoda Pro Screen"));
	g_fontFamilyList.push_back(QString("VWThesis MIB Light Arabic"));
	g_fontFamilyList.push_back(QString("VWThesis MIB Arabic"));
#endif

	for(FontFamilyList::iterator iter = g_fontFamilyList.begin(); iter != g_fontFamilyList.end(); ++ iter)
	{
		ui.comboBoxFontFamily->addItem(*iter);
	}
}

void TestPango::init_pango_enums()
{
	g_pangoWrapModeList.push_back( std::make_pair(PANGO_WRAP_WORD, "WRAP_WORD"));
	g_pangoWrapModeList.push_back( std::make_pair(PANGO_WRAP_CHAR, "WRAP_CHAR"));
	g_pangoWrapModeList.push_back( std::make_pair(PANGO_WRAP_WORD_CHAR, "WORD_CHAR"));

	g_pangoEllipsizeModeList.push_back( std::make_pair(PANGO_ELLIPSIZE_NONE, "ELLIPSIZE_NONE"));
	g_pangoEllipsizeModeList.push_back( std::make_pair(PANGO_ELLIPSIZE_START, "ELLIPSIZE_START"));
	g_pangoEllipsizeModeList.push_back( std::make_pair(PANGO_ELLIPSIZE_MIDDLE, "ELLIPSIZE_MIDDLE"));
	g_pangoEllipsizeModeList.push_back( std::make_pair(PANGO_ELLIPSIZE_END, "ELLIPSIZE_END"));

	g_pangoAlignmentList.push_back( std::make_pair(PANGO_ALIGN_RIGHT, "ALIGN_RIGHT"));
	g_pangoAlignmentList.push_back( std::make_pair(PANGO_ALIGN_LEFT, "ALIGN_LEFT"));
	g_pangoAlignmentList.push_back( std::make_pair(PANGO_ALIGN_CENTER, "ALIGN_CENTER"));


	for(PangoWrapModeList::iterator iter = g_pangoWrapModeList.begin(); iter != g_pangoWrapModeList.end(); ++ iter)
	{
		ui.comboBoxWrap->addItem(iter->second);
	}

	for(PangoEllipsizeModeList::iterator iter = g_pangoEllipsizeModeList.begin(); iter != g_pangoEllipsizeModeList.end(); ++ iter)
	{
		ui.comboBoxEllipse->addItem(iter->second);
	}

	for(PangoAlignmentList::iterator iter = g_pangoAlignmentList.begin(); iter != g_pangoAlignmentList.end(); ++ iter)
	{
		ui.comboBoxAlignment->addItem(iter->second);
	}
}

/**********************************************************************************************
 *
 **********************************************************************************************/
static size_t convert_wchar_string_to_utf8_string(const wchar_t *wchar_string, char* utf8_buffer, size_t utf8_buffer_size)
{
	assert(utf8_buffer && utf8_buffer_size);

	if(!wchar_string || !*wchar_string) return 0;

	size_t wchar_string_byte_length = wcslen(wchar_string) * sizeof(wchar_t);
	size_t org_utf8_buffer_size = utf8_buffer_size;

	iconv_t cd = iconv_open("UTF-8", "WCHAR_T");
	assert(cd != (iconv_t)(-1));

	size_t ret = iconv(cd, (const char **)&wchar_string, &wchar_string_byte_length, (char **)&utf8_buffer, &utf8_buffer_size); 
	assert(ret != (size_t)(-1));

	iconv_close(cd);

	return (org_utf8_buffer_size - utf8_buffer_size);
}

static void init_fonts()
{
	FcBool ret;

	// FcConfig* config = FcConfigCreate();
	FcConfig* config = FcConfigGetCurrent();
	assert(config);

#if 0
	// Not working... ???
	ret = FcConfigAppFontAddDir(config, (const FcChar8 *)"./font");
	assert(ret != FcFalse);
#else
	ret = FcConfigAppFontAddFile(config, (const FcChar8 *)"./font/SkodaProScreen-Regular.ttf");
	assert(ret != FcFalse);

	ret = FcConfigAppFontAddFile(config, (const FcChar8 *)"./font/VWThesisMIBArabic_150622.ttf");
	assert(ret != FcFalse);

	ret = FcConfigAppFontAddFile(config, (const FcChar8 *)"./font/VWThesisMIBArabic-Light_150622.ttf");
	assert(ret != FcFalse);

		ret = FcConfigAppFontAddFile(config, (const FcChar8 *)"./font/Hangul-Hei_C.ttf");
	assert(ret != FcFalse);

	ret = FcConfigAppFontAddFile(config, (const FcChar8 *)"./font/JisX0213-2004-Hei_c(20160428).ttf");
	assert(ret != FcFalse);

	ret = FcConfigAppFontAddFile(config, (const FcChar8 *)"./font/ZYHei_GB18030_c(20160919).ttf");
	assert(ret != FcFalse);
#endif

	FcConfigSetCurrent(config);

	s_fontmap = (PangoCairoFontMap*)pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
	assert(s_fontmap != NULL);
	pango_cairo_font_map_set_default(s_fontmap);

	s_pango_context = pango_cairo_font_map_create_context(s_fontmap);
	assert(s_pango_context != NULL);
}

static PangoFontDescription* get_font_desc(const char *family, float size)
{
	PangoFontDescription* font_desc = NULL;

	font_desc = pango_font_description_copy(pango_context_get_font_description(s_pango_context));
	if(font_desc)
	{
		pango_font_description_set_family_static(font_desc, family);
		pango_font_description_set_style(font_desc, PANGO_STYLE_NORMAL);
		pango_font_description_set_stretch(font_desc, PANGO_STRETCH_NORMAL);
		pango_font_description_set_weight(font_desc, PANGO_WEIGHT_NORMAL);
		pango_font_description_set_size(font_desc, (int) (size * PANGO_SCALE));
	}

	return font_desc;
}


/* first line of speller */

void TestPango::appendText1()
{
	//park
	str[idx++] = QChar(0x3131);
	onApplSpeller();
}

void TestPango::appendText2()
{
	str[idx++] = QChar(0x3134);
	onApplSpeller();
}

void TestPango::appendText3()
{
	str[idx++] = QChar(0x3137);
	onApplSpeller();
}

void TestPango::appendText4()
{
	str[idx++] = QChar(0x3139);
	onApplSpeller();
}
void TestPango::appendText5()
{
	str[idx++] = QChar(0x3141);
	onApplSpeller();
}
void TestPango::appendText6()
{
	str[idx++] = QChar(0x3142);
	onApplSpeller();
}
void TestPango::appendText7()
{
	str[idx++] = QChar(0x3145);
	onApplSpeller();
}
void TestPango::appendText8()
{
	str[idx++] = QChar(0x3147);
	onApplSpeller();
}
void TestPango::appendText9()
{
	str[idx++] = QChar(0x3148);
	onApplSpeller();
}
void TestPango::appendText10()
{
	str[idx++] = QChar(0x314A);
	onApplSpeller();
}
void TestPango::appendText44()
{
	str[idx++] = QChar(0x314B);
	onApplSpeller();
}
void TestPango::appendText45()
{
	idx = 0;
	str.clear();
	onApplSpeller();
}

/* second line of speller */

void TestPango::appendText11()
{
	//park
	str[idx++] = QChar(0x314C);
	onApplSpeller();
}

void TestPango::appendText12()
{
	str[idx++] = QChar(0x314D);
	onApplSpeller();
}

void TestPango::appendText13()
{
	str[idx++] = QChar(0x314E);
	onApplSpeller();
}

void TestPango::appendText14()
{
	str[idx++] = QChar(0x3132);
	onApplSpeller();
}
void TestPango::appendText15()
{
	str[idx++] = QChar(0x3138);
	onApplSpeller();
}
void TestPango::appendText16()
{
	str[idx++] = QChar(0x3143);
	onApplSpeller();
}
void TestPango::appendText17()
{
	str[idx++] = QChar(0x3146);
	onApplSpeller();
}
void TestPango::appendText18()
{
	str[idx++] = QChar(0x3149);
	onApplSpeller();
}
void TestPango::appendText19()
{
	str[idx++] = QChar(0x314F);
	onApplSpeller();
}
void TestPango::appendText20()
{
	str[idx++] = QChar(0x3150);
	onApplSpeller();
}

void TestPango::appendText21()
{
	//park
	str[idx++] = QChar(0x3151);
	onApplSpeller();
}

void TestPango::appendText22()
{
	str[idx++] = QChar(0x3153);
	onApplSpeller();
}

/* third line of speller */

void TestPango::appendText23()
{
	str[idx++] = QChar(0x3154);
	onApplSpeller();
}

void TestPango::appendText24()
{
	str[idx++] = QChar(0x3155);
	onApplSpeller();
}
void TestPango::appendText25()
{
	str[idx++] = QChar(0x3157);
	onApplSpeller();
}
void TestPango::appendText26()
{
	str[idx++] = QChar(0x3158);
	onApplSpeller();
}
void TestPango::appendText27()
{
	str[idx++] = QChar(0x315B);
	onApplSpeller();
}
void TestPango::appendText28()
{
	str[idx++] = QChar(0x315C);
	onApplSpeller();
}
void TestPango::appendText29()
{
	str[idx++] = QChar(0x3160);
	onApplSpeller();
}
void TestPango::appendText30()
{
	str[idx++] = QChar(0x3161);
	onApplSpeller();
}

void TestPango::appendText31()
{
	//park
	str[idx++] = QChar(0x3162);
	onApplSpeller();
}

void TestPango::appendText32()
{
	str[idx++] = QChar(0x3163);
	onApplSpeller();
}

void TestPango::appendText33()
{
	str[idx++] = QChar(0xAC00);
	onApplSpeller();
}

/* forth line of speller */

void TestPango::appendText34()
{
	str[idx++] = QChar(0xAC01);
	onApplSpeller();
}
void TestPango::appendText35()
{
	str[idx++] = QChar(0xAC02);
	onApplSpeller();
}
void TestPango::appendText36()
{
	str[idx++] = QChar(0xAC07);
	onApplSpeller();
}
void TestPango::appendText37()
{
	str[idx++] = QChar(0xAC04);
	onApplSpeller();
}
void TestPango::appendText38()
{
	str[idx++] = QChar(0xAC08);
	onApplSpeller();
}
void TestPango::appendText39()
{
	str[idx++] = QChar(0xAC09);
	onApplSpeller();
}
void TestPango::appendText40()
{
	str[idx++] = QChar(0xAC10);
	onApplSpeller();
}
void TestPango::appendText41()
{
	str[idx++] = QChar(0xAC11);
	onApplSpeller();
}
void TestPango::appendText42()
{
	str[idx++] = QChar(0xAC12);
	onApplSpeller();
}
void TestPango::appendText43()
{
	str[idx++] = QChar(0xAC13);
	onApplSpeller();
}

/* fifth Line of speller */
void TestPango::appendText46()
{
	str[idx++] = QChar(0x0041);
	onApplSpeller();
}
void TestPango::appendText47()
{
	str[idx++] = QChar(0x0042);
	onApplSpeller();
}
void TestPango::appendText48()
{
	str[idx++] = QChar(0x0043);
	onApplSpeller();
}
void TestPango::appendText49()
{
	str[idx++] = QChar(0x0031);
	onApplSpeller();
}
void TestPango::appendText50()
{
	str[idx++] = QChar(0x0032);
	onApplSpeller();
}
void TestPango::appendText51()
{
	str[idx++] = QChar(0x0033);
	onApplSpeller();
}