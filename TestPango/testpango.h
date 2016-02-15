#ifndef TESTPANGO_H
#define TESTPANGO_H

#include <QtGui/QMainWindow>
#include "ui_testpango.h"

class TestPango : public QMainWindow
{
	Q_OBJECT

public:
	TestPango(QWidget *parent = 0, Qt::WFlags flags = 0);
	~TestPango();

private:
	void paintEvent(QPaintEvent * event);
	void drawImage();
	void update_attributes_to_ui();
	void update_attributes_from_ui();
	void update_text();

	void init_font_family();
	void init_pango_enums();
	void onApplyAttributes();
	void onApplSpeller();

private slots:
	void onApply();
//	void onApplyAttributes();
	void onOpenUTF8File();
	void appendText1();
	void appendText2();
	void appendText3();
	void appendText4();
	void appendText5();
	void appendText6();
	void appendText7();
	void appendText8();
	void appendText9();
	void appendText10();
	void appendText11();
	void appendText12();
	void appendText13();
	void appendText14();
	void appendText15();
	void appendText16();
	void appendText17();
	void appendText18();
	void appendText19();
	void appendText20();
	void appendText21();
	void appendText22();
	void appendText23();
	void appendText24();
	void appendText25();
	void appendText26();
	void appendText27();
	void appendText28();
	void appendText29();
	void appendText30();
	void appendText31();
	void appendText32();
	void appendText33();
	void appendText34();
	void appendText35();
	void appendText36();
	void appendText37();
	void appendText38();
	void appendText39();
	void appendText40();
	void appendText41();
	void appendText42();
	void appendText43();
	void appendText44();
	void appendText45();
	void appendText46();
	void appendText47();
	void appendText48();
	void appendText49();
	void appendText50();
	void appendText51();

private:
	Ui::TestPangoClass ui;

	QImage m_image;
	QString str;
	int idx;
	bool isSpellerEnter;
};

#endif // TESTPANGO_H
