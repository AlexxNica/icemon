#include "mon-kde.h"

#include <qsocketnotifier.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstdaction.h>
#include <kdebug.h>
#include "ganttstatusview.h"

#include <qcanvas.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qvaluelist.h>
#include <math.h>

#include <iostream>
#include <comm.h>
#include <cassert>

using namespace std;

class NodeItem : public QCanvasText
{
public:
    NodeItem( const QString &hostName, QCanvas *canvas )
        : QCanvasText( hostName, canvas ),
          m_hostName( hostName ),
          m_stateItem( 0 )
        {
        }

    void setState( Job::State state ) { m_state = state; }
    Job::State state() const { return m_state; }

    void setStateItem( QCanvasItem *item ) { m_stateItem = item; }
    QCanvasItem *stateItem() { return m_stateItem; }

    QString hostName() const { return m_hostName; }

private:
    Job::State m_state;
    QString m_hostName;
    QCanvasItem *m_stateItem;
};

QString Job::stateAsString() const
{
    switch ( m_state ) {
    case WaitingForCS:
        return "Waiting";
        break;
    case Compiling:
        return "Compiling";
        break;
    case Finished:
        return "Finished";
        break;
    case Failed:
        return "Failed";
        break;
    case Idle:
        return "Idle";
        break;
    case LocalOnly:
        return "LocalOnly";
        break;
    }
    return QString::null;
}

void StatusView::checkNode( const QString &, unsigned int )
{
}

ListStatusViewItem::ListStatusViewItem( QListView *parent, const Job &_job )
    :  QListViewItem( parent ), job( _job )
{
    updateText( job );
}

void ListStatusViewItem::updateText( const Job &job)
{
    this->job = job;
    setText( 0, QString::number( job.jobId() ) );
    setText( 1, job.fileName() );
    setText( 2, job.client() );
    setText( 3, job.server() );
    setText( 4, job.stateAsString() );
    setText( 5, QString::number( job.real_msec ) );
    setText( 6, QString::number( job.user_msec ) );
    setText( 7, QString::number( job.majflt ) );
    setText( 8, QString::number( job.in_uncompressed ) );
    setText( 9, QString::number( job.out_uncompressed ) );
}

inline int compare( unsigned int i1, unsigned int i2 )
{
    if ( i1 < i2 )
        return -1;
    else if ( i1 == i2 )
        return 0;
    else
        return 1;
}

int ListStatusViewItem::compare( QListViewItem *i, int col,
                                 bool ) const
{
    const ListStatusViewItem *first = this;
    const ListStatusViewItem *other = dynamic_cast<ListStatusViewItem*>( i );
    assert( other );

    switch ( col ) {
    case 0:
        return ::compare( first->job.jobId(), other->job.jobId() );
    case 5:
        return ::compare( first->job.real_msec, other->job.real_msec );
    case 6:
        return ::compare( first->job.user_msec, other->job.user_msec );
    case 7:
        return ::compare( first->job.majflt, other->job.majflt );
    case 8:
        return ::compare( first->job.in_uncompressed, other->job.in_uncompressed );
    case 9:
        return ::compare( first->job.out_uncompressed, other->job.out_uncompressed );
    default:
        return first->text(col).compare( other->text( col ) );
    }
}

ListStatusView::ListStatusView( QWidget *parent, const char *name )
	: KListView( parent, name )
{
    addColumn( i18n( "ID" ) );
    addColumn( i18n( "Filename" ) );
    addColumn( i18n( "Client" ) );
    addColumn( i18n( "Server" ) );
    addColumn( i18n( "State" ) );
    addColumn( i18n( "Real" ) );
    addColumn( i18n( "User" ) );
    addColumn( i18n( "Faults" ) );
    addColumn( i18n( "Size In" ) );
    addColumn( i18n( "Size Out" ) );
}

void ListStatusView::update( const Job &job )
{
    ItemMap::iterator it = items.find( job.jobId() );
    if ( it == items.end() )
    {
        items[job.jobId()] = new ListStatusViewItem( this, job );

    } else
    {
        ( *it )->updateText( job );
    }
}

#if 0
StarStatusView::StarStatusView( QWidget *parent, const char *name )
	: StatusView( parent, name, WRepaintNoErase | WResizeNoErase )
{
    m_canvas = new QCanvas( this );
    m_canvas->resize( width(), height() );

    QHBoxLayout *layout = new QHBoxLayout( this );
    layout->setMargin( 0 );

    m_canvasView = new QCanvasView( m_canvas, this );
    m_canvasView->setVScrollBarMode( QScrollView::AlwaysOff );
    m_canvasView->setHScrollBarMode( QScrollView::AlwaysOff );
    layout->addWidget( m_canvasView );

    m_localhostItem = new QCanvasText( i18n( "localhost" ), m_canvas );
    centerLocalhostItem();
    m_localhostItem->show();

    m_canvas->update();
}

void StarStatusView::update( const JobList &jobs )
{
    checkForNewNodes( jobs );
    updateNodeStatus( jobs );
    drawNodeStatus();
}

void StarStatusView::resizeEvent( QResizeEvent * )
{
    m_canvas->resize( width(), height() );
    centerLocalhostItem();
    arrangeNodeItems();
    m_canvas->update();
}

void StarStatusView::centerLocalhostItem()
{
    const QRect br = m_localhostItem->boundingRect();
    const int newX = ( width() - br.width() ) / 2;
    const int newY = ( height() - br.height() ) / 2;
    m_localhostItem->move( newX, newY );
}

void StarStatusView::arrangeNodeItems()
{
    const int radius = kMin( m_canvas->width() / 2, m_canvas->height() / 2 );
    const double step = 2 * M_PI / m_nodeItems.count();

    double angle = 0.0;
    QDictIterator<NodeItem> it( m_nodeItems );
    while ( it.current() != 0 ) {
        it.current()->move( m_localhostItem->x() + ( cos( angle ) * radius ),
                            m_localhostItem->y() + ( sin( angle ) * radius ) );
        angle += step;
        ++it;
    }
}

void StarStatusView::checkForNewNodes( const JobList &jobs )
{
    bool newNode = false;

    JobList::ConstIterator it = jobs.begin();
    for ( ; it != jobs.end(); ++it ) {
        if ( m_nodeItems.find( ( *it ).host() ) == 0 ) {
            NodeItem *nodeItem = new NodeItem( ( *it ).host(), m_canvas );
            m_nodeItems.insert( ( *it ).host(), nodeItem );
            nodeItem->show();
            newNode = true;
        }
    }

    if ( newNode ) {
        arrangeNodeItems();
        m_canvas->update();
    }
}

void StarStatusView::updateNodeStatus( const JobList &jobs )
{
    QDictIterator<NodeItem> it( m_nodeItems );
    while ( it.current() != 0 ) {
        JobList::ConstIterator jobIt = jobs.begin();
        it.current()->setState( Job::Unknown );
        for ( ; jobIt != jobs.end(); ++jobIt ) {
            if ( it.current()->hostName() == ( *jobIt ).host() )
                it.current()->setState( ( *jobIt ).state() );
        }
        ++it;
    }
}

void StarStatusView::drawNodeStatus()
{
    QDictIterator<NodeItem> it( m_nodeItems );
    for ( ; it.current() != 0; ++it )
        drawState( it.current() );
    m_canvas->update();
}

void StarStatusView::drawState( NodeItem *node )
{
	delete node->stateItem();
	QCanvasItem *newItem = 0;

	const QPoint nodeCenter = node->boundingRect().center();
	const QPoint localCenter = m_localhostItem->boundingRect().center();

	switch ( node->state() ) {
		case Job::Compile: {
			QCanvasLine *line = new QCanvasLine( m_canvas );
			line->setPen( Qt::green );

			line->setPoints( nodeCenter.x(), nodeCenter.y(),
			                 localCenter.x(), localCenter.y() );
			line->show();
			newItem = line;
			break;
		}
		case Job::CPP: {
			QCanvasLine *line = new QCanvasLine( m_canvas );
			line->setPen( QPen( Qt::darkGreen, 0, QPen::DashLine ) );

			line->setPoints( nodeCenter.x(), nodeCenter.y(),
			                 localCenter.x(), localCenter.y() );
			line->show();
			newItem = line;
			break;
		}
		case Job::Send: {
			QPointArray points( 3 );
			points.setPoint( 0, localCenter.x() - 5, localCenter.y() );
			points.setPoint( 1, nodeCenter );
			points.setPoint( 2, localCenter.x() + 5, localCenter.y() );

			QCanvasPolygon *poly = new QCanvasPolygon( m_canvas );
			poly->setBrush( Qt::blue );
			poly->setPoints( points );
			poly->show();

			newItem = poly;
			break;
		}
		case Job::Receive: {
			QPointArray points( 3 );
			points.setPoint( 0, nodeCenter.x() - 5, nodeCenter.y() );
			points.setPoint( 1, localCenter );
			points.setPoint( 2, nodeCenter.x() + 5, nodeCenter.y() );

			QCanvasPolygon *poly = new QCanvasPolygon( m_canvas );
			poly->setBrush( Qt::blue );
			poly->setPoints( points );
			poly->show();

			newItem = poly;
			break;
		}
	}

	node->setStateItem( newItem );
}
#endif

MainWindow::MainWindow( QWidget *parent, const char *name )
	: KMainWindow( parent, name ), m_view( 0 ), m_scheduler( 0 ),
          m_scheduler_read( 0 )
{
    KRadioAction *a = new KRadioAction( i18n( "&List View" ), 0,
                                        this, SLOT( setupListView() ),
                                        actionCollection(), "view_list_view" );
    a->setExclusiveGroup( "viewmode" );

    a = new KRadioAction( i18n( "&Star View" ), 0,
                          this, SLOT( setupStarView() ),
                          actionCollection(), "view_star_view" );
    a->setExclusiveGroup( "viewmode" );

    a = new KRadioAction( i18n( "&Gantt View" ), 0,
                          this, SLOT( setupGanttView() ),
                          actionCollection(), "view_gantt_view" );
    a->setExclusiveGroup( "viewmode" );

    KStdAction::quit( kapp, SLOT( quit() ), actionCollection() );

    new KAction( i18n("Stop"), 0, this, SLOT( stopView() ), actionCollection(),
                 "view_stop" );

    new KAction( i18n("Start"), 0, this, SLOT( startView() ),
                 actionCollection(), "view_start" );

    new KAction( i18n("Check Nodes"), 0, this, SLOT( checkNodes() ),
                 actionCollection(), "check_nodes" );

    createGUI();
    setupGanttView();
    checkScheduler();

    setAutoSaveSettings();
}

void MainWindow::checkScheduler(bool deleteit)
{
    if ( deleteit ) {
        delete m_scheduler;
        m_scheduler = 0;
        delete m_scheduler_read;
        m_scheduler_read = 0;
    } else if ( m_scheduler )
        return;
    QTimer::singleShot( 300, this, SLOT( slotCheckScheduler() ) );
}

void MainWindow::slotCheckScheduler()
{
    std::list<std::string> names = get_netnames (60);
    if ( !names.empty() && m_current_netname.isEmpty() )
    {
        m_current_netname = names.front().c_str();
    }

    if ( m_current_netname.isEmpty() ) {
        checkScheduler( true );
        return;
    }

    m_scheduler = connect_scheduler ( m_current_netname.latin1() );
    if ( m_scheduler ) {
        if ( !m_scheduler->send_msg (MonLoginMsg()) )
        {
            checkScheduler( true );
            return;
        }
        m_scheduler_read = new QSocketNotifier( m_scheduler->fd,
                                                QSocketNotifier::Read,
                                                this );
        QObject::connect( m_scheduler_read, SIGNAL(activated(int)),
                          SLOT( msgReceived()) );
    } else {
        checkScheduler( true );
    }
}

void MainWindow::msgReceived()
{
    Msg *m = m_scheduler->get_msg ();
    if ( !m ) {
        kdDebug() << "lost connection to scheduler\n";
        checkScheduler(true);
        return;
    }

    switch (m->type) {
    case M_MON_GET_CS:
        handle_getcs( m );
        break;
    case M_MON_JOB_BEGIN:
        handle_job_begin( m );
        break;
    case M_MON_JOB_DONE:
        handle_job_done( m );
        break;
    case M_END:
        cout << "END" << endl;
        checkScheduler( true );
        break;
    case M_MON_STATS:
        handle_stats( m );
        break;
    case M_MON_LOCAL_JOB_BEGIN:
        handle_local_begin( m );
        break;
    case M_MON_LOCAL_JOB_DONE:
        handle_local_done( m );
        break;
    default:
        cout << "UNKNOWN" << endl;
        break;
    }
    delete m;
}

void MainWindow::handle_getcs(Msg *_m)
{
    MonGetCSMsg *m = dynamic_cast<MonGetCSMsg*>( _m );
    if ( !m )
        return;
    m_rememberedJobs[m->job_id] = Job( m->job_id, m->client.c_str(),
                                       m->filename.c_str(),
                                       m->version.c_str(),
                                       m->lang == CompileJob::Lang_C ? "C" : "C++" );
    m_view->update( m_rememberedJobs[m->job_id] );
}

void MainWindow::handle_local_begin( Msg *_m )
{
    MonLocalJobBeginMsg *m = dynamic_cast<MonLocalJobBeginMsg*>( _m );
    if ( !m )
        return;

    m_rememberedJobs[m->job_id] = Job( m->job_id, m->host.c_str(),
                                       QString::null,
                                       QString::null,
                                       "C++" );
    m_rememberedJobs[m->job_id].setState( Job::LocalOnly );
    m_view->update( m_rememberedJobs[m->job_id] );
}

void MainWindow::handle_local_done( Msg *_m )
{
    MonLocalJobDoneMsg *m = dynamic_cast<MonLocalJobDoneMsg*>( _m );
    if ( !m )
        return;
    JobList::iterator it = m_rememberedJobs.find( m->job_id );
    if ( it == m_rememberedJobs.end() ) // we started in between
        return;

    ( *it ).exitcode = m->exitcode;
    ( *it ).setState( Job::Finished );
    m_view->update( *it );
}

void MainWindow::handle_stats( Msg *_m )
{
    MonStatsMsg *m = dynamic_cast<MonStatsMsg*>( _m );
    if ( !m )
        return;
#if 0
    cout << "load"
         << " host=" << m->host
         << " iceload=" << m->load
         << " niceLoad=" << m->niceLoad
         << " sysLoad=" << m->sysLoad
         << " userLoad=" << m->userLoad
         << " idleLoad=" << m->idleLoad
         << " loadavg=(" << m->loadAvg1
         << ", " << m->loadAvg5 << ", " << m->loadAvg10 << ")"
         << " freemem=" << m->freeMem
         << endl;
#endif

  m_view->checkNode( m->host.c_str(), m->max_kids );
}

void MainWindow::handle_job_begin(Msg *_m)
{
    MonJobBeginMsg *m = dynamic_cast<MonJobBeginMsg*>( _m );
    if ( !m )
        return;
    JobList::iterator it = m_rememberedJobs.find( m->job_id );
    if ( it == m_rememberedJobs.end() ) // we started in between
        return;
    ( *it ).setServer( m->host.c_str() );
    ( *it ).setStartTime( m->stime );
    ( *it ).setState( Job::Compiling );

#if 0
    kdDebug() << "BEGIN: " << (*it).fileName() << " (" << (*it).jobId()
              << ")" << endl;
#endif

    m_view->update( *it );
}

void MainWindow::handle_job_done(Msg *_m)
{
    MonJobDoneMsg *m = dynamic_cast<MonJobDoneMsg*>( _m );
    if ( !m )
        return;
    JobList::iterator it = m_rememberedJobs.find( m->job_id );
    if ( it == m_rememberedJobs.end() ) // we started in between
        return;

    ( *it ).exitcode = m->exitcode;
    if ( m->exitcode )
        ( *it ).setState( Job::Failed );
    else {
        ( *it ).setState( Job::Finished );
        ( *it ).real_msec = m->real_msec;
        ( *it ).user_msec = m->user_msec;
        ( *it ).sys_msec = m->sys_msec;   /* system time used */
        ( *it ).maxrss = m->maxrss;     /* maximum resident set size (KB) */
        ( *it ).idrss = m->idrss;      /* integral unshared data size (KB) */
        ( *it ).majflt = m->majflt;     /* page faults */
        ( *it ).nswap = m->nswap;      /* swaps */

        ( *it ).in_compressed = m->in_compressed;
        ( *it ).in_uncompressed = m->in_uncompressed;
        ( *it ).out_compressed = m->out_compressed;
        ( *it ).out_uncompressed = m->out_uncompressed;
    }

#if 0
    kdDebug() << "DONE: " << (*it).fileName() << " (" << (*it).jobId()
              << ")" << endl;
#endif

    m_view->update( *it );
}

void MainWindow::setupView( StatusView *view, bool rememberJobs )
{
    delete m_view;
    m_view = view;
    if ( rememberJobs ) {
      JobList::ConstIterator it = m_rememberedJobs.begin();
      for ( ; it != m_rememberedJobs.end(); ++it )
          m_view->update( *it );
    }
    setCentralWidget( m_view->widget() );
    m_view->widget()->show();
}

void MainWindow::setupListView()
{
    setupView( new ListStatusView( this ), true );
}

void MainWindow::setupGanttView()
{
    setupView( new GanttStatusView( this ), false );
}

void MainWindow::setupStarView()
{
//	setupView( new StarStatusView( this ), false );
}

void MainWindow::stopView()
{
  m_view->stop();
}

void MainWindow::startView()
{
  m_view->start();
}

void MainWindow::checkNodes()
{
  m_view->checkNodes();
}

void MainWindow::setCurrentNet( const QString &netName )
{
  m_current_netname = netName;
}

const char * rs_program_name = "icemon";
const char * const appName = I18N_NOOP( "icemon" );
const char * const version = "0.1";
const char * const description = I18N_NOOP( "distcc monitor for KDE" );
const char * const copyright = I18N_NOOP( "(c) 2003, Frerich Raabe <raabe@kde.org>" );
const char * const bugsEmail = "raabe@kde.org";

static const KCmdLineOptions options[] =
{
  { "n", 0, 0 },
  { "netname <name>", "Icecream network name", 0 },
  KCmdLineLastOption
};

int main( int argc, char **argv )
{
	KAboutData aboutData( rs_program_name, appName, version, description,
	                      KAboutData::License_BSD, copyright, bugsEmail );
	KCmdLineArgs::init( argc, argv, &aboutData );
        KCmdLineArgs::addCmdLineOptions( options );

	KApplication app;
	MainWindow *mainWidget = new MainWindow( 0 );

        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
        QString netName = QString::fromLocal8Bit( args->getOption( "netname" ) );
        if ( !netName.isEmpty() ) {
          mainWidget->setCurrentNet( netName );
        }

	app.setMainWidget( mainWidget );
	mainWidget->show();

    	return app.exec();
}

#include "mon-kde.moc"
