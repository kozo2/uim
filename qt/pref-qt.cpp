/*

 Copyright (c) 2003,2004,2005 uim Project http://uim.freedesktop.org/

 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. Neither the name of authors nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGE.

*/
#include "pref-qt.h"
#include "pref-customwidgets.h"
#include "pref-kseparator.h"

#include <qvbox.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qtoolbutton.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include <qsplitter.h>
#include <qlistview.h>
#include <qvbox.h>
#include <qspinbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qfiledialog.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qobjectlist.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qsettings.h>

#include "uim/config.h"
#include "qtgettext.h"

#include <stdlib.h>
#include <locale.h>

#define _FU8(String) QString::fromUtf8(String)

UimPrefDialog::UimPrefDialog( QWidget *parent, const char *name )
    : QDialog( parent, name ),
      m_isValueChanged( false )
{
    uim_init();
    if (uim_custom_enable()) {
        checkDotUimFile();        
        setupWidgets();
    } else {
        qDebug("uim_custom_enable() failed.");
        uim_quit();
        QApplication::exit( -1 );
    }

    setCaption( "uim-pref-qt" );
}

UimPrefDialog::~UimPrefDialog()
{
    uim_quit();
}

void UimPrefDialog::checkDotUimFile()
{
    /* Check Config */
    QSettings settings;
    bool isShowOnStartup = settings.readBoolEntry( "/uim/qt/warnDotUim", true );
    if( !isShowOnStartup )
        return;

    /* Check File Existance */
    QString homeDir = getenv( "HOME" );
    if( homeDir.isEmpty() )
        return;

    QString dotUim = homeDir + "/.uim";
    if( QFile::exists( dotUim ) )
    {
        QString msg = _("The user customize file \"~/.uim\" is found.\n"
                        "This file will override all conflicted settings set by\n"
                        "this tool (stored in ~/.uim.d/customs/*.scm).\n"
                        "Please check the file if you find your settings aren't applied.");

        QConfirmDialog *d = new QConfirmDialog( msg,
                                                "/uim/qt/warnDotUim",
                                                this );
        d->setCaption( _("~/.uim exists!") );
        d->exec();
        delete d;            
    }
}

/*
 * Building up GUI
 */
void UimPrefDialog::setupWidgets()
{
    createMainWidgets();
    createGroupWidgets();
}

void UimPrefDialog::createMainWidgets()
{
    QVBoxLayout *mainVLayout = new QVBoxLayout( this );
    mainVLayout->setMargin( 6 );

    QSplitter *mainSplitter = new QSplitter( this );

    /* ListView */
    m_groupListView = new QListView( mainSplitter );
    m_groupListView->addColumn( _("Group") );
    m_groupListView->setSelectionMode( QListView::Single );
    m_groupListView->setSorting( -1 );
    QObject::connect( m_groupListView, SIGNAL(selectionChanged( QListViewItem * )),
                      this, SLOT(slotSelectionChanged( QListViewItem * )) );

    /* Contents Frame */
    QWidget *rightSideWidget = new QWidget( mainSplitter );
    QVBoxLayout *rightVLayout = new QVBoxLayout( rightSideWidget );
    m_groupWidgetStack = new QWidgetStack( rightSideWidget );
    rightVLayout->setSpacing( 6 );
    rightVLayout->addWidget( m_groupWidgetStack );

    /* Buttons */
    QWidget *buttonHWidget = new QWidget( this );
    QHBoxLayout *buttonHLayout = new QHBoxLayout( buttonHWidget );
    buttonHLayout->setMargin( 6 );
    buttonHLayout->setSpacing( 6 );
    QPushButton *defaultButton = new QPushButton( _("Defaults"), buttonHWidget );
    QObject::connect( defaultButton, SIGNAL(clicked()),
                      this, SLOT(slotSetDefault()) );
    QPushButton *okButton = new QPushButton( _("OK"), buttonHWidget );
    QObject::connect( okButton, SIGNAL(clicked()),
                      this, SLOT(slotOK()) );
    m_applyButton = new QPushButton( _("Apply"), buttonHWidget );
    m_applyButton->setEnabled( false );
    QObject::connect( m_applyButton, SIGNAL(clicked()),
                      this, SLOT(slotApply()) );
    QPushButton *cancelButton = new QPushButton( _("Cancel"), buttonHWidget );
    QObject::connect( cancelButton, SIGNAL(clicked()),
                      this, SLOT(slotCancel()) );
    buttonHLayout->addWidget( defaultButton );
    buttonHLayout->addStretch();
    buttonHLayout->addWidget( okButton );
    buttonHLayout->addWidget( m_applyButton );
    buttonHLayout->addWidget( cancelButton );

    mainVLayout->addWidget( mainSplitter );
    mainVLayout->addWidget( new KSeparator( this ) );
    mainVLayout->addWidget( buttonHWidget );
}

void UimPrefDialog::createGroupWidgets()
{
    char **primary_groups = uim_custom_primary_groups();
    char **grp = NULL;
    for( grp = primary_groups; *grp; grp++ )
    {
        struct uim_custom_group *group = uim_custom_group_get( *grp );
        if( group == NULL )
            continue;

        /* insert item in uim's order */
        QListViewItem *item = NULL;
        QListViewItem *lastItem = m_groupListView->lastItem();
        if( lastItem )
            item = new QListViewItem( m_groupListView, lastItem, _FU8(group->label) );
        else
            item = new QListViewItem( m_groupListView, _FU8(group->label) );

        GroupPageWidget *w = new GroupPageWidget( m_groupWidgetStack, *grp );
        QObject::connect( w, SIGNAL(customValueChanged()),
                          this, SLOT(slotCustomValueChanged()) );

        m_groupWidgetsDict.insert( _FU8(group->label), w );
        m_groupWidgetStack->addWidget( w );

        uim_custom_group_free( group );
    }
    uim_custom_symbol_list_free( primary_groups );
}

/*
 * GUI event handling
 */
void UimPrefDialog::slotSelectionChanged( QListViewItem * item )
{
    /*
     * Don't confirm on each change in slot selection according to
     * [Anthy-dev 1795].
     * Dec 09 2005 ekato
     */
#if 0
    /* confirm if save the change */
    if( m_isValueChanged )    
        confirmChange();
#endif
    
    /* switch group widget */
    QString grpname = item->text( 0 );
    m_groupWidgetStack->raiseWidget( m_groupWidgetsDict[grpname] );

#if 0
    m_applyButton->setEnabled( false );
#endif
}

void UimPrefDialog::slotCustomValueChanged()
{
    m_isValueChanged = true;
    m_applyButton->setEnabled( true );
}

void UimPrefDialog::confirmChange()
{
    int result = QMessageBox::question( this,
                                        _("Save Confirm"),
                                        _("The value was changed.\nSave?"),
                                        _("OK"),
                                        _("Cancel") );
    switch(result)
    {
    case 0:
        slotApply();
        break;
    case 1:
        m_isValueChanged = false;
        break;
    }
}

void UimPrefDialog::confirmQuit()
{
    int result = QMessageBox::question( this,
                                        _("Quit Confirm"),
                                        _("Some value(s) have been changed.\n"
                                          "Do you really quit this program?"),
                                        _("Yes"),
                                        _("No"),
                                        QString::null, 1, -1);
    switch(result)
    {
    case 0:
        reject();
        break;
    case 1:
        break;
    default:
        break;
    }
}

void UimPrefDialog::slotSetDefault()
{
    QWidget *w = m_groupWidgetStack->visibleWidget();
    if( w )
    {
        ((GroupPageWidget*)w)->setDefault();
    }
}

void UimPrefDialog::slotApply()
{
    if( !m_isValueChanged )
        return;

    qDebug("start saving....");

    uim_custom_save();
    uim_custom_broadcast_reload_request();

    m_isValueChanged = false;
    m_applyButton->setEnabled( false );
}

void UimPrefDialog::slotOK()
{
    if( m_isValueChanged )
    {
        slotApply();
    }
    accept();
}

void UimPrefDialog::slotCancel()
{
    /*
     * Enable confirmation since each change in slot selection is not
     * checked strictly according to [Anthy-dev 1795].
     * Dec 09 2005 ekato
     */
    if( m_isValueChanged )
        confirmQuit();
    else
        reject();
}

//-------------------------------------------------------------------------------------
QConfirmDialog::QConfirmDialog( const QString &msg, const QString &confname, QWidget *parent, const char *name )
    : QDialog( parent, name ),
      m_confname( confname )
{
    QSettings settings;
    bool isShowOnStartup = settings.readBoolEntry( m_confname, true );
    if( isShowOnStartup )
        setupWidgets( msg );
}

void QConfirmDialog::setupWidgets( const QString&msg )
{
    QVBoxLayout *vLayout = new QVBoxLayout( this );
    vLayout->setSpacing( 6 );
    vLayout->setMargin( 6 );
    QLabel *msgLabel = new QLabel( msg, this );
    KSeparator *sep = new KSeparator( this );
    vLayout->addWidget( msgLabel );
    vLayout->addWidget( sep );

    QHBoxLayout *buttonHLayout = new QHBoxLayout( vLayout );
    QCheckBox *checkBox = new QCheckBox( _("Show this dialog on startup"), this );
    QSettings settings;
    bool isWarnDotUim = settings.readBoolEntry( m_confname, true );
    checkBox->setChecked( isWarnDotUim );
    QPushButton *ok = new QPushButton( _("OK"), this );
    ok->setDefault(true);
    buttonHLayout->addWidget( checkBox );
    buttonHLayout->addStretch();    
    buttonHLayout->addWidget( ok );

    QObject::connect( ok, SIGNAL(clicked()),
                      this, SLOT(accept()) );
    QObject::connect( checkBox, SIGNAL(toggled(bool)),
                      this, SLOT(showOnStart(bool)) );
}

void QConfirmDialog::showOnStart( bool isShowOnStart )
{
    QSettings settings;
    settings.writeEntry( m_confname, isShowOnStart );

    qDebug("show on start = %d", isShowOnStart );
}


//-----------------------------------------------------------------------------------

GroupPageWidget::GroupPageWidget( QWidget *parent, const char *group_name )
    : QWidget( parent )
{
    m_customIfaceList.clear();
    m_customIfaceList.setAutoDelete( false );
    
    setupWidgets( group_name );
}

void GroupPageWidget::setupWidgets( const char *group_name )
{
    QVBoxLayout *vLayout = new QVBoxLayout( this );
    vLayout->setMargin( 6 );
    vLayout->setSpacing( 3 );
    
    struct uim_custom_group *group = uim_custom_group_get( group_name );
    if( group == NULL )
        return;

    QLabel *groupLabel = new QLabel( _FU8(group->label), this );
    groupLabel->setAlignment( Qt::AlignLeft );
    vLayout->addWidget( groupLabel );
    
    KSeparator *separator = new KSeparator( this );
    vLayout->addWidget( separator );

    /* default QVGroupBox */
    QVGroupBox *defaultGroupVBox = new QVGroupBox( this );
    vLayout->addWidget( defaultGroupVBox );    
    defaultGroupVBox->hide();

#if 0
    SubgroupData *sd = new SubgroupData( this, group_name );

    /* add various widgets to the vbox */
    char **custom_syms = uim_custom_collect_by_group( group_name );
    if( custom_syms )
    {
        for( char **custom_sym = custom_syms; *custom_sym; custom_sym++ )
        {
            QVGroupBox *vbox = sd->searchGroupVBoxByCustomSym( *custom_sym );
            if( vbox == NULL )
            {
                vbox = defaultGroupVBox;
                vbox->show();
            }
            
            UimCustomItemIface *iface = addCustom( vbox, *custom_sym );
            if( iface )
                m_customIfaceList.append( iface );
        }

        uim_custom_symbol_list_free( custom_syms );
    }
#else
    char **sub_groups = uim_custom_group_subgroups( group_name );
    char **sgrp;
    for( sgrp = sub_groups; *sgrp; sgrp++ )
    {
        struct uim_custom_group *sgroup_custom =  uim_custom_group_get( *sgrp );
        QVGroupBox *vbox;
        if( QString::compare( *sgrp, "main" ) == 0 )
        {
            vbox = defaultGroupVBox;
	    vbox->show();
        }
        else
        {
            vbox = new QVGroupBox( _FU8(sgroup_custom->label), this );
            layout()->add( vbox );
        }

	/* XXX quick hack to use AND expression of groups */
	QString groups( group_name );
	groups += " '";
	groups += *sgrp;
        char **custom_syms = uim_custom_collect_by_group( groups );
        if( !custom_syms )
            continue;

        for( char **custom_sym = custom_syms; *custom_sym; custom_sym++ )
        {
            UimCustomItemIface *iface = addCustom( vbox, *custom_sym );
            if( iface )
                m_customIfaceList.append( iface );
        }
        uim_custom_symbol_list_free( custom_syms );


        uim_custom_group_free( sgroup_custom );
    }
    uim_custom_symbol_list_free( sub_groups );
#endif

    /* free */
    //delete sd;
    uim_custom_group_free( group );

    /* bottom up */
    vLayout->addStretch();    
}

/*
 * Building up GUI in accordance with Custom Type.
 */
UimCustomItemIface *GroupPageWidget::addCustom( QVGroupBox *vbox, const char *custom_sym )
{
    UimCustomItemIface *w = NULL;
    struct uim_custom *custom = uim_custom_get( custom_sym );
    if( custom )
    {
        switch( custom->type )
        {
        case UCustom_Bool:
            w = addCustomTypeBool( vbox, custom );
            break;
        case UCustom_Int:
            w = addCustomTypeInteger( vbox, custom );
            break;
        case UCustom_Str:
            w = addCustomTypeString( vbox, custom );
            break;
        case UCustom_Pathname:
            w = addCustomTypePathname( vbox, custom );
            break;
        case UCustom_Choice:
            w = addCustomTypeChoice( vbox, custom );
            break;
        case UCustom_OrderedList:
            w = addCustomTypeOrderedList( vbox, custom );
            break;
        case UCustom_Key:
            w = addCustomTypeKey( vbox, custom );
            break;
        default:
            w = NULL;
            qWarning( "Invalid custom type: %d\n", custom->type );
            uim_custom_free( custom );
            break;
        }
    } else {
        qWarning( "Failed to get uim_custom object for %s.", custom_sym );
    }

    /* custom is freed by UimCustomItemIface's destructor */

    return w;
}

UimCustomItemIface *GroupPageWidget::addCustomTypeBool( QVGroupBox *vbox, struct uim_custom *custom )
{
    CustomCheckBox *checkBox = new CustomCheckBox( custom, vbox );
    QObject::connect( checkBox, SIGNAL(customValueChanged()),
                      this, SLOT(slotCustomValueChanged()) );

    return checkBox;
}

UimCustomItemIface *GroupPageWidget::addCustomTypeInteger( QVGroupBox *vbox, struct uim_custom *custom )
{
    QHBox *hbox = new QHBox( vbox );
    hbox->setSpacing( 6 );
    QLabel *label = new QLabel( _FU8(custom->label), hbox );
    hbox->setStretchFactor( new QWidget( hbox ), 1 );
    CustomSpinBox *spinBox = new CustomSpinBox( custom, hbox );
    label->setBuddy( spinBox );

    QObject::connect( spinBox, SIGNAL(customValueChanged()),
                      this, SLOT(slotCustomValueChanged()) );

    return spinBox;
}

UimCustomItemIface *GroupPageWidget::addCustomTypeString( QVGroupBox *vbox, struct uim_custom *custom )
{
    QHBox *hbox = new QHBox( vbox );
    hbox->setSpacing( 6 );
    QLabel *label = new QLabel( _FU8(custom->label) + ":", hbox );
    CustomLineEdit *lineEdit = new CustomLineEdit( custom, hbox );
    label->setBuddy( lineEdit );

    QObject::connect( lineEdit, SIGNAL(customValueChanged()),
                      this, SLOT(slotCustomValueChanged()) );

    return lineEdit;
}

UimCustomItemIface *GroupPageWidget::addCustomTypePathname( QVGroupBox *vbox, struct uim_custom *custom )
{
    QHBox *hbox = new QHBox( vbox );
    hbox->setSpacing( 6 );
    QLabel *label = new QLabel( _FU8(custom->label), hbox );
    CustomPathnameEdit *pathnameEdit = new CustomPathnameEdit( custom, hbox );
    label->setBuddy( pathnameEdit );

    QObject::connect( pathnameEdit, SIGNAL(customValueChanged()),
                      this, SLOT(slotCustomValueChanged()) );

    return pathnameEdit;
}

UimCustomItemIface *GroupPageWidget::addCustomTypeChoice( QVGroupBox *vbox, struct uim_custom *custom )
{
    QHBox *hbox = new QHBox( vbox );
    hbox->setSpacing( 6 );
    QLabel *label = new QLabel( _FU8(custom->label), hbox );

    CustomChoiceCombo *choiceCombo = new CustomChoiceCombo( custom, hbox );
    label->setBuddy( choiceCombo );

    QObject::connect( choiceCombo, SIGNAL(customValueChanged()),
                      this, SLOT(slotCustomValueChanged()) );

    return choiceCombo;
}

UimCustomItemIface *GroupPageWidget::addCustomTypeOrderedList( QVGroupBox *vbox, struct uim_custom *custom )
{
    QHBox *hbox = new QHBox( vbox );
    hbox->setSpacing( 6 );
    QLabel *label = new QLabel( _FU8(custom->label), hbox );
    CustomOrderedListEdit *olistEditBox = new CustomOrderedListEdit( custom, hbox );
    label->setBuddy( olistEditBox );

    QObject::connect( olistEditBox, SIGNAL(customValueChanged()),
                      this, SLOT(slotCustomValueChanged()) );

    return olistEditBox;
}

UimCustomItemIface *GroupPageWidget::addCustomTypeKey( QVGroupBox *vbox, struct uim_custom *custom )
{
    QHBox *hbox = new QHBox( vbox );
    hbox->setSpacing( 6 );
    QLabel *label = new QLabel( _FU8(custom->label), hbox );
    CustomKeyEdit *keyEditBox = new CustomKeyEdit( custom, hbox );
    label->setBuddy( keyEditBox );
    QObject::connect( keyEditBox, SIGNAL(customValueChanged()),
                      this, SLOT(slotCustomValueChanged()) );

    return keyEditBox;
}

void GroupPageWidget::setDefault()
{
    for( UimCustomItemIface *iface = m_customIfaceList.first();
         iface;
         iface = m_customIfaceList.next() )
    {
        iface->setDefault();
    }
}

//--------------------------------------------------------------------------------------
int main( int argc, char **argv )
{
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    bind_textdomain_codeset(PACKAGE, "UTF-8"); // ensure code encoding is UTF8-
    
    QApplication a( argc, argv );

    UimPrefDialog *dlg = new UimPrefDialog();
    a.setMainWidget( dlg );
    dlg->show();

    return a.exec();
}

#include "pref-qt.moc"
