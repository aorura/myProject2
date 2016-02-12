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

private slots:
	void onApplyAttributes();
	void onOpenUTF8File();

private:
	Ui::TestPangoClass ui;

	QImage m_image;
};

#endif // TESTPANGO_H
