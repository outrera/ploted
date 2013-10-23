/********************************************************
* Ploted - Plot Editor
* Copyright (C) 2013   Nikita Vadimovich Sadkov
*
* This software is provided only as part of my portfolio.
* All rights belong to ЗАО «НПП «СКИЗЭЛ»
*********************************************************/

#ifndef MAIN_H
#define MAIN_H

#include <QtGui>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QModelIndex>
#include <QStandardItem>
#include <QDialog>

#include <qwt_scale_map.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_math.h>
#include <qwt_plot.h>
#include "common.h"


class QAction;
class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QMenu;
class QMenuBar;
class QPushButton;
class QToolButton;
class QTextEdit;
class QCheckBox;
class QCustomPlot;
class QVBoxLayout;
class QFormLayout;
class QScrollBar;
class QTreeView;
class QTabWidget;
class QListWidget;
class QStandardItemModel;
class QModelIndex;
class QComboBox;

class QwtPlot;
class QwtPlotZoomer;
class QwtPlotPanner;
class QwtPlotGrid;
class QwtPlotCurve;

class QxtSpanSlider;


class curve;
class plot;
class operation;
class projectView;
class mainWindow;
class curveItemModel;

typedef QVector<curve*> curves;


QString trn(char const *M, char const *Context = 0);


#define model_foreach(Index,Base, M, Body) do { \
    QStack<QModelIndex> _Stack_; \
    _Stack_.push(Base); \
    while (_Stack_.size()) { \
        QModelIndex Index = _Stack_.pop(); \
        times(_I_, M->rowCount(Index)) _Stack_.push(M->index(_I_, 0, Index)); \
        unless (Index.isValid()) continue; \
        Body; \
    } \
    } while(0)

class curve {
public:
    curve(ptr<operation> Op);
    curve(); // group
    ~curve();

    flts xs() const {return Xs;}
    flts ys() const {return Ys;}
    size_t size() const {return Xs.size();}
    int id() {return Id;}
    QString name() {return Name;}
    void setName(QString NewName) {Name = NewName;}
    QString source() {return Source;}
    void setSource(QString NewSource) {Source = NewSource;}
    QString description() {return Description;}
    void setDescription(QString NewDescription) {Description = NewDescription;}
    QString channel() {return Channel;}
    void setChannel(QString NewChannel) {Channel = NewChannel;}
    ptr<operation> op() {return Op;}
    void setOp(ptr<operation> NewOp);
    double min() const {return Min;}
    double max() const {return Max;}
    double average() const {return Average;}
    double minAbs() const {return MinAbs;}
    double maxAbs() const {return MaxAbs;}
    double averageAbs() const {return AverageAbs;}
    curves parents();
    curves groups() {return Groups;}
    curves kids() {return Kids;}
    curves publicKids() {return PublicKids;}
    int show() {return Show;}
    void setShow(int State);
    QColor color() {return Color;}
    void setColor(QColor NewColor);
    ptr<QwtPlotCurve> plotCurve() {return PlotCurve;}
    void attach(plot *Plot);
    void detach();
    QVector<QModelIndex> modelIndices() {return ModelIndices;}
    void addModelIndex(QModelIndex Index) {ModelIndices.append(Index);}
    void removeModelIndex(QModelIndex Index) {ModelIndices = keep(I,ModelIndices,I!=Index);}
    bool isGroup();
    bool isDependent();
    void setRemoved(bool State) {Removed = State;}
    bool removed() {return Removed;}

    void removeKid(curve* C);
    void addToGroup(curve* G);

    QVector<QString> properties();
    QString propertyName(QString Id);
    double propertyValue(QString Id);

    void update();

private:
    void addKid(curve* C, bool Public=true);
    void doInit(ptr<operation> Op);

    flts Xs, Ys; // curve's Xs and Ys
    QString Name; // custom name
    QString Source; // where it came from
    QString Channel; // channel inside the source
    QString Description;
    ptr<operation> Op; // what operation produced this curve
    curves Groups; // groups, this curve is member of
    curves Parents; //traces the path this data came from
    curves Kids; // all curves based on this one
    curves PublicKids; // kids shown in curve tree, under this curve
    int Show; //how to display plot in a plot view
    double Min, Max; // minimum and maximum amplitudes
    double Average;
    double MinAbs, MaxAbs; // min and max of abs(Y)
    double AverageAbs;
    QColor Color;
    int Id;
    ptr<QwtPlotCurve> PlotCurve;
    QwtPlotZoomer *Zoomer;
    QVector<QModelIndex> ModelIndices;
    bool Removed;
};
#define make_curve(Args) (new curve(Args))
#define make_op(Type,Args...) ptr<operation>(new op_##Type(Args))

#define PICK_CURVE 0x1
#define PICK_GROUP 0x2

class itemPickerDialog : public QDialog {
    Q_OBJECT
public:
    itemPickerDialog(curveItemModel *M, QWidget *Parent = 0, int PickWhat=PICK_CURVE);
    ~itemPickerDialog();
    QModelIndex pickedIndex() {return PickedIndex;}

private Q_SLOTS:
    void ok();
    void cancel();
    void curvePicked(const QModelIndex&, const QModelIndex&);

private:
    curveItemModel *CurveListModel;
    QTreeView *View;
    QModelIndex PickedIndex;
    QPushButton *OkButton;
    bool CurvesAllowed, GroupsAllowed;
};

// dependency picker diaglo
class depPickerDialog : public QDialog {
    Q_OBJECT
public:
    depPickerDialog(curve *C, curves Cs, QWidget *Parent = 0);
    //QModelIndex pickedIndex() {return PickedIndex;}

private Q_SLOTS:
    void ok();
    void cancel();
    //void curvePicked(const QModelIndex&, const QModelIndex&);

public Q_SLOTS:
    void curvePicked(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void curveItemChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);


private:
    void populateTree(QStandardItem *Parent, curve *C);

    curveItemModel *Model;
    QTreeView *View;
    //QModelIndex PickedIndex;
    QPushButton *OkButton;
};


class operation {
public:
    virtual ~operation() {}
    virtual QString name() = 0;
    virtual QString type() = 0;
    virtual QString dynamicName();
    virtual int arity() {return 1;}
    virtual int arityMax() {return arity();}
    virtual curves operands() {return Cs;}
    virtual void setOperands(curves Curves) {Cs = Curves;}
    virtual QVector<flts> apply() = 0; //apply operator to input curves

    QVector<QString> args() {return Args;}
    QString argName(QString Id) {return ArgsNames[Id];}
    QString argDefault(QString Id) {return ArgsDefaults[Id];}
    QString argValue(QString Id) {return ArgsValues[Id];}
    void setArgValue(QString Id, QString Value) {ArgsValues[Id] = Value;}
    double argAsFloat(QString Id) {
        QString Prop = argCurveProperty(Id);
        if (Prop == "") return argValue(Id).toDouble();
        return argCurve(Id)->propertyValue(Prop);
    }
    curve *argCurve(QString Id) {return ArgsCurves[Id];}
    void setArgCurve(QString Id, curve *Value) {ArgsCurves[Id] = Value;}
    QString argCurveProperty(QString Id) {return ArgsCurvesProperties[Id];}
    void setArgCurveProperty(QString Id, QString Value) {ArgsCurvesProperties[Id] = Value;}
    int argAllowsCurve(QString Id) {return ArgsAllowsCurve[Id];}

protected:
    void addArg(QString Id, QString Name, QString DefaultValue, bool AllowsCurve=false) {
        Args.append(Id);
        ArgsNames[Id] = Name;
        ArgsDefaults[Id] = DefaultValue;
        ArgsValues[Id] = DefaultValue;
        ArgsAllowsCurve[Id] = AllowsCurve;
    }
    QVector<curve*> Cs; // input curves

private:
    QVector<QString> Args;
    QHash<QString,QString> ArgsNames;
    QHash<QString,QString> ArgsDefaults;
    QHash<QString,QString> ArgsValues;
    QHash<QString,curve*> ArgsCurves;
    QHash<QString,QString> ArgsCurvesProperties;
    QHash<QString,int> ArgsAllowsCurve;
};

QVector<ptr<operation> > availableOperations();

class operationPicker : public QDialog {
    Q_OBJECT
public:
    operationPicker(QVector<ptr<operation> > Ops, QWidget *Parent = 0);
    ptr<operation> pickedOp() {return PickedOp;}

private Q_SLOTS:
    void ok();
    void cancel();

private:
    ptr<operation> PickedOp;
    QVector<ptr<operation> > Ops;
    QListWidget *View;
};

class curveEditor : public QDialog {
    Q_OBJECT
public:
    curveEditor(ptr<operation> Op, curveItemModel *Model, QWidget *Parent = 0);
    curves operands() {return Curves;}
    void setOperands(curves Cs);

private Q_SLOTS:
    void changeCurve();
    void pickArgumentCurve();
    void pickPropertyCurve();
    void addInputCurve(curve *C = 0);
    void switchValueSource(bool State);
    void ok();
    void cancel();

private:
    ptr<operation> Op;
    curveItemModel *CurveListModel;
    QVector<QPushButton*> Buttons; //arguments are displayed as buttons
    QVector<QLabel*> ButtonsLabels;
    curves Curves;
    QFormLayout *InputCurvesLayout;

    QHash<QString,QLineEdit*> ArgsLineEdits;
    QHash<QString,QPushButton*> ArgsCurvesButtons;
    QHash<QString,curve*> ArgsCurves;
    QHash<QString,QComboBox*> ArgsComboBoxes;

    QPushButton *OkButton;
    QPushButton *AddInputCurveButton;
};

class projectInfoTab : public QTableWidget {
    Q_OBJECT
public:
    projectInfoTab();
    void updateInfo(projectView* P);

private:
    void addRow(QString Name, QString Value);
    int Count;
};

class curveInfoTab : public QTableWidget {
    Q_OBJECT
public:
    curveInfoTab();
    void updateInfo(curve* C);

private:
    void addRow(QString Name, QString Value);
    int Count;
};

class customTabBar : public QTabBar {
    Q_OBJECT
public:
    customTabBar(QWidget *Parent = 0);
    void mouseDoubleClickEvent(QMouseEvent *E);

Q_SIGNALS:
    void textChanged(int Index);

private:
};

class tabWidget : public QTabWidget {
    Q_OBJECT
public:
    tabWidget(QWidget *Parent = 0);
    void showNewTabButton(bool State);
    QTabBar *tabBar() {return QTabWidget::tabBar();}

Q_SIGNALS:
    void textChanged(int Index);
    void newTabRequested();

private:
    customTabBar *TabBar;
    QToolButton *NewTabButton;
};

class plot : public QWidget {
    Q_OBJECT
public:
    plot(QWidget *Parent = 0);
    QHash<int,int> attachedCurves() {return AttachedCurves;}
    void setAttachedCurves(QHash<int,int> As) {AttachedCurves = As;}
    //void resizeEvent(QResizeEvent *event);
    void setScale(double MinX, double MaxX, double MinY, double MaxY);
    void replot();

    QwtPlot *qwtPlot() {return Plot;}

Q_SIGNALS:
    void customContextMenuRequested(const QPoint&);

private:
    QwtPlot *Plot;
    QHash<int,int> AttachedCurves;
    QwtPlotZoomer *Zoomer;
    QwtPlotPanner *Panner;
    QwtPlotGrid *Grid;
    QxtSpanSlider *XSlider, *YSlider;
    QLineEdit *XMin, *YMin, *XMax, *YMax;
};

class curveItemModel : public QStandardItemModel {
public:
    curveItemModel(QObject *Parent);
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole);
    void setEditable(bool State);
};


class projectView : public QWidget {
    Q_OBJECT

    prop(QString, Name)
    prop(QString, Description)
    prop(QString, Filename)
public:
    projectView(QWidget *parent = 0);
    ~projectView();
    void replot();
    curves getCurves() {return Curves;}
    void addCurve(curve* C);
    void addCurveUnder(curve* C, QModelIndex ParentIndex);
    void updateCurveList();
    void addToGroupIndexed(QModelIndex RootIndex, curve* C);

    bool initFromFile();

    curve *addDependentCurveFromCurves(curves Cs);

public Q_SLOTS:
    void curvePicked(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void curveItemChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void import();
    void save();
    void saveProject();
    void saveProjectAs();
    void removeCurve();
    void removeCurveById(int Id, bool UpdateList = 1);
    void addDependentCurveFromNothing();
    void addDependentCurveFromContext();
    void addDependentCurveFromSenderId();
    void newGroup();
    void generate();
    void showCurveListContextMenu(const QPoint&);
    void showPlotContextMenu(const QPoint&);
    void showCurveInfo();
    void newPlot(bool MakeCurrent = true);
    void removePlot();
    void plotTabChanged(int Index);
    void plotTabClose(int Index);
    void renameItem();
    void createCurveUnderGroup();
    void addSubgroupToGroup();
    void toggleCurvePlotVisibility();
    void createProcessingModule();
    void editCurve();
    void moveCurveToGroup();
    void reloadCurveFromFile();

private:
    void syncPlotAttachedCurves();

    tabWidget *PlotTabs;
    QTreeView *CurveListView;
    curveItemModel *CurveListModel;
    int CurveCount;
    int PlotCount;
    int GroupCount;
    curves Curves;
    QVector<plot*> Plots;
    plot *Plot; //current plot
    QVector<operation*> Ops;
    projectInfoTab *ProjectInfoTab;
    curveInfoTab *CurveInfoTab;
    QPushButton *DuplicatePlotButton;
};

class mainWindow : public QMainWindow {
    Q_OBJECT
public:
    mainWindow(QWidget *parent = 0);
    ~mainWindow();

private Q_SLOTS:
    void openProject();
    void saveProject();
    void saveProjectAs();
    void newProject();
    void removeProject();
    void projectTabChanged(int Index);
    void projectTabClose(int Index);
    void projectTabRenamed(int Index);

private:
    int ProjectCount;
    tabWidget *ProjectTabs;
    projectView *Project; //current project
};

#endif // MAIN_H
