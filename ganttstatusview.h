#ifndef GANTTSTATUSVIEW_H
#define GANTTSTATUSVIEW_H

#include "mon-kde.h"

#include <qmap.h>
#include <qvaluelist.h>
#include <qwidget.h>

class Job;
class JobList;
class QGridLayout;
class QTimer;

class GanttTimeScaleWidget : public QWidget
{
	Q_OBJECT
	public:
		GanttTimeScaleWidget( QWidget *parent, const char *name = 0 );

                void setPixelsPerSecond( int );

	protected:
		virtual void paintEvent( QPaintEvent *e );

        private:
                int mPixelsPerSecond;
};

class GanttProgress : public QWidget
{
	Q_OBJECT
	public:
		GanttProgress( QMap<QString,QColor> &hostColors,
                               QWidget *parent, const char *name = 0 );

                void setHostColors( QMap<QString,QColor> & );

                bool isFree() const { return mIsFree; }
                bool fullyIdle() const { return m_jobs.count() == 1 && isFree(); }

	public slots:
		void progress();
		void update( const Job &job );

	protected:
		virtual void paintEvent( QPaintEvent *e );
		virtual void resizeEvent( QResizeEvent *e );

	private:
		void adjustGraph();
		void drawGraph( QPainter &p );
		QColor colorForStatus( const Job &job ) const;

                struct JobData
                {
                    JobData( const Job& j, int c )
                        : job( j ), clock( c ), next_text_width( 0 ) {}
                    Job job;
                    int clock;
                    mutable int next_text_width;
                    mutable QPixmap text_cache;
                    JobData() {}; // stupid QValueList
                    bool operator==( const JobData& d )
                        { return job == d.job && clock == d.clock; }
                };
		QValueList< JobData > m_jobs;
                
                QMap<QString,QColor> &mHostColors;

                int mClock;

                bool mIsFree;
};

class GanttStatusView : public QWidget, public StatusView
{
    Q_OBJECT

public:
    GanttStatusView( QWidget *parent, const char *name = 0 );
    virtual ~GanttStatusView() {}

    void checkNode( const QString &host, unsigned int max_kids );

    void start();
    void stop();

public slots:
    virtual void update( const Job &job );
    virtual QWidget *widget();

private slots:
    void updateGraphs();
    void checkAge();

private:
    GanttProgress *registerNode( const QString &name );
    void removeSlot( const QString& name, GanttProgress* slot );
    void unregisterNode( const QString &name );
    void createHostColor( const QString &host );

    QGridLayout *m_topLayout;
    typedef QValueList<GanttProgress *> SlotList;
    typedef QMap<QString,SlotList> NodeMap;
    NodeMap mNodeMap;
    typedef QMap<QString,int> AgeMap;
    AgeMap mAgeMap;
    typedef QMap<unsigned int, GanttProgress *> JobMap;
    JobMap mJobMap;
    typedef QMap<QString,QVBoxLayout *> NodeLayoutMap;
    NodeLayoutMap mNodeLayouts;
    typedef QMap<QString,int> NodeRowMap;
    NodeRowMap mNodeRows;
    QTimer *m_progressTimer;
    QTimer *m_ageTimer;

    QMap<QString,QColor> mHostColors;

    bool mRunning;

    int mUpdateInterval;
};

#endif // GANTTSTATUSVIEW_H
// vim:ts=4:sw=4:noet
