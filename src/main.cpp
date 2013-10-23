/********************************************************
* Ploted - Plot Editor
* Copyright (C) 2013   Nikita Vadimovich Sadkov
*
* This software is provided only as part of my portfolio.
* All rights belong to ЗАО «НПП «СКИЗЭЛ»
*********************************************************/

#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>

//#include <QxtSpanSlider>

#include "main.h"

static flts calcDerivative(flts Xs, flts Ys) {
    int N = Xs.size();
    double DX,DY,DA,DB;
    flts Zs(N); //result

    DX = Xs[1]-Xs[0];
    DY = Ys[1]-Ys[0];
    DA = DY/DX;
    for (int I = 1; I < N-1; I++) {
        DX = Xs[I+1]-Xs[I];
        DY = Ys[I+1]-Ys[I];
        DB = DY/DX;
        Zs[I] = (DA+DB)/2;
        DA = DB;
    }

    //HACK: extrapolating derivative
    Zs[0  ] = Zs[1  ] - (Zs[2  ]-Zs[1  ]);
    Zs[N-1] = Zs[N-2] + (Zs[N-2]-Zs[N-3]);
    return Zs;
}

class op_empty : public operation {
public:
    QString type() {return "generate";}
    QString name() {return trn("Generate");}
    QVector<flts> apply() {return vec(flts(4), flts(4));}
};


class op_constant : public operation {
public:
    op_constant() : Xs(flts(4)), Ys(flts(4)) {}
    op_constant(flts Xs, flts Ys) : Xs(Xs), Ys(Ys) {}
    QString type() {return "constant";}
    QString name() {return trn("Constant");}
    QVector<flts> apply() {return vec(Xs, Ys);}

private:
    flts Xs, Ys;
};


class op_group : public operation {
public:
    QString type() {return "group";}
    QString name() {return trn("Group");}
    QVector<flts> apply() {return vec(flts(4), flts(4));}
};

class op_generate : public operation {
public:
    op_generate(QString What, int Size) : What(What), Size(Size) {}
    QString type() {return "generate";}
    QString name() {return trn("Generate");}
    QVector<flts> apply() {
        const double DX = 0.1;
        flts Xs, Ys;
        times (I, Size) {
            double X = I*DX;
            double Y;
            if (What == "sin") Y = sin(X);
            else {
                QMessageBox::information(0, "error", trn("op_gen: invalid function=") + What);
                abort();
            }
            Xs.append(X);
            Ys.append(Y);
        }
        return vec(Xs, Ys);
    }

private:
    QString What;
    int Size;
};

class op_file : public operation {
public:
    op_file() {
        addArg("filename", trn("Filename"), "");
    }
    op_file(QString FileName) {
        addArg("filename", trn("Filename"), "");
        setArgValue("filename", FileName);
    }
    QString type() {return "file";}
    QString name() {return trn("Load File");}
    QVector<flts> apply() {
        flts Xs, Ys;
        QString FileName(argValue("filename"));
        QFile F(FileName);
        unless (F.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::information(0, "error", F.errorString());
            return vec(flts(3), flts(3));
        }
        QTextStream S(&F);
        QString Ext = QFileInfo(FileName).suffix().toLower();
        if (Ext == "afc") {
            while (!F.atEnd()) {
                QString L = S.readLine();
                times (I, L.size()) {
                    if (L[I] == '\x0d'|| L[I] == '\x0a') L[I] = ' ';
                }
                if (L == "    END_SECTION_PARAMS ") break;
            }
            till (F.atEnd()) {
                QString L = S.readLine();
                QChar LastChar = ' ';
                times (I, L.size()) {
                    QChar C = L[I];
                    if (LastChar != ' ' && L[I] == ' ') L[I] = ',';
                    LastChar = C;
                }
                int Col = 0;
                each(Field, L.split(",")) {
                    if (Col == 0) Xs.append((double)Xs.size()); //Xs.append(Field.toDouble());
                    else if (Col == 1) Ys.append(Field.toDouble());
                    else {
                        // ignore additional fields
                    }
                    Col++;
                }
            }
        } else {
            till(F.atEnd()) {
                int Col = 0;
                each(Field, S.readLine().split(",")) {
                    if (Col == 0) Xs.append((double)Xs.size()); //Xs.append(Field.toDouble());
                    else if (Col == 1) Ys.append(Field.toDouble());
                    else {
                        // ignore additional fields
                    }
                    Col++;
                }
            }
        }

        if (Ys.size() < 3) {
            QMessageBox::information(0, trn("error"), trn("Too Few Points"));
            return vec(flts(3), flts(3));
        }

        F.close();
        return vec(Xs, Ys);
    }
};

class op_diff : public operation {
public:
    QString type() {return "diff";}
    QString name() {return trn("Differentiate");}
    QVector<flts> apply() {
        let(In, Cs[0]);
        flts Xs = In->xs();
        flts Ys = In->ys();
        flts Zs = calcDerivative(Xs, Ys);
        return vec(Xs, Zs);
    }
};

class op_int : public operation {
public:
    QString type() {return "int";}
    QString name() {return trn("Integrate");}
    QVector<flts> apply() {
        let(In, Cs[0]);
        flts Xs = In->xs();
        flts Ys = In->ys();
        int N = Ys.size();
        flts Zs(N);
        double Sum = Zs[0] = Ys[0] * (Xs[1] - Xs[0]) / 2;
        for (int I = 1; I < N-1; I++) {
            Sum += Ys[I] * (Xs[I+1] - Xs[I-1]) / 2;
            Zs[I] = Sum;
        }
        Sum += Ys[N-1] * (Xs[N-1] - Xs[N - 2]) / 2;
        Zs[N-1] = Sum;
        return vec(Xs, Zs);
    }
};

class op_sum : public operation {
public:
    QString type() {return "sum";}
    QString name() {return trn("Sum");}
    int arity() {return 2;}
    int arityMax() {return 1<<16;} //any number of arguments
    QVector<flts> apply() {
        int N = -1;
        flts Xs;
        times (J, Cs.size()) {
            int NewN = Cs[J]->xs().size();
            if (N < NewN) {
              N = NewN;
              Xs = Cs[J]->xs();
            }
        }
        flts Zs(N);

        //FIXME: find matching BXs and interpolate
        times (J, Cs.size()) {
            flts Ys = Cs[J]->ys();
            times (I, Xs.size()) Zs[I] += Ys[I];
        }
        return vec(Xs, Zs);
    }
};

class op_neg : public operation {
public:
    QString type() {return "neg";}
    QString name() {return trn("Negate");}
    QVector<flts> apply() {return vec(Cs[0]->xs(), map(Y,Cs[0]->ys(),-Y));}
};

// analyze function determines peaks, using provided theresholds
// it also determines maximum amplitude
class op_peaks : public operation {
public:
    QString type() {return "peaks";}
    QString name() {return trn("Find Peaks");}
    QVector<flts> apply() {
        let(In, Cs[0]);
        flts Xs = In->xs();
        flts Ys = In->ys();
        int N = In->size();
        flts Zs(N); //result

        int I, K, J, PeakNum = 1;
        double Q = 0.020; //PeakThreshold->text().toDouble(); //detection threshold
        double S = 0.030; //CutoffThreshold->text().toDouble();
        double MaxAmplitude = In->max();

        flts Ds = calcDerivative(Xs, Ys);
        QVector<ints> Lifts;
        ints L;
        ints Hs(N);
        for (I = 1; I < N-1; I++) {
            double A = Ds[I-1];
            double B = Ds[I+1];
            if (sign(A) != sign(B)) { // stationary point?
                if (A > 0 && abs(A)/MaxAmplitude > Q && abs(B)/MaxAmplitude > Q) { // maxumum?
                    // calculate maximum ascend and descend derivatives
                    double MaxA = abs(A);
                    for (J = I-1, K=3; K-- && J > 0 && Ds[J] > 0; J--) if (abs(Ds[J]) > MaxA) MaxA = abs(Ds[J]);
                    double MaxB = abs(B);
                    for (J = I+1, K=3; K-- && J < N-1 && Ds[J] < 0; J++) if (abs(Ds[J]) > MaxB) MaxB = abs(Ds[J]);
                    if (abs(MaxA)/MaxAmplitude > Q && abs(MaxB)/MaxAmplitude > Q) { // peak?
                        //mark peak backward and forward down to breaking points
                        Hs[I] = Hs[I] ? -PeakNum : PeakNum;
                        for (J = I-1; J > 0 && Ds[J] > 0; J--) {
                            Hs[J] = Hs[J] ? -PeakNum : PeakNum;
                            double A = Ds[J-1];
                            double B = Ds[J+1];
                            if (I-J > 3 && abs(B/A) > S) break;
                        }
                        for (J = I+1; J < N-1 && Ds[J] < 0; J++) {
                            Hs[J] = Hs[J] ? -PeakNum : PeakNum;
                            double A = Ds[J-1];
                            double B = Ds[J+1];
                            if (J-I > 2 && abs(B/A) > S) break;
                        }
                        PeakNum++;
                    }
                }
                Lifts.append(L);
                L = ints();
                I += 1;
            }
            L.append(I);
        }

        times (I, Zs.size()) {
            int H = Hs[I]; // get current peak number
                           // if H is negative, then point is shared between two peaks
            unless(H) continue;
            Zs[I] = 1.0;
        }
        return vec(Xs, Zs);
    }
};

class op_amplify : public operation {
public:
    op_amplify() {addArg("scale", trn("Scale"), "2.0");}
    QString type() {return "amp";}
    QString name() {return trn("Amplify");}
    QVector<flts> apply() {
        let(In, Cs[0]);
        double Scale = argValue("scale").toDouble();
        return vec(In->xs(), map(Y, In->ys(), Y*Scale));
    }
};

#define EPSILON 0.000001

class op_filter : public operation {
public:
    op_filter() {
        addArg("threshold", trn("Threshold"), "1.0", true);
        addArg("comparison_type", trn("Comparison Type")
              ,"~greater|greater or equal|less|less or equal|equal|not equal");
        addArg("result_type", trn("Result Type"), "~1/0|value/0");
        addArg("curve_for_result", trn("Curve for result"), "<curve>");
    }
    QString type() {return "filter";}
    QString name() {return trn("Filter");}
    QVector<flts> apply() {
        let(In, Cs[0]);
        QString Threshold = argValue("threshold");
        QString C = argValue("comparison_type");
        QString RT = argValue("result_type");
        flts Ys = In->ys();
        flts Ts;
        if (Threshold == "<values>") Ts = argCurve("threshold")->ys();
        else {
            double T = argAsFloat("threshold");
            Ts = map(Y, Ys, T);
        }
        flts Zs;
        if (RT == "1/0") Zs = map(Y, Ys, 1.0);
        else {
            curve* CFR = argCurve("curve_for_result");
            Zs = CFR ? CFR->ys() : In->ys();
        }
        int I = -1;
        if (C == "greater")               Ys = map(Y, Ys, ++I; Y>Ts[I] ? Zs[I] : 0.0);
        else if (C == "greater or equal") Ys = map(Y, Ys, ++I; Y>=Ts[I] ? Zs[I] : 0.0);
        else if (C == "less")             Ys = map(Y, Ys, ++I; Y<Ts[I] ? Zs[I] : 0.0);
        else if (C == "less or equal")    Ys = map(Y, Ys, ++I; Y<=Ts[I] ? Zs[I] : 0.0);
        else if (C == "equal")            Ys = map(Y, Ys, ++I; abs(Y-Ts[I])<EPSILON ? Zs[I] : 0.0);
        else if (C == "not equal")        Ys = map(Y, Ys, ++I; abs(Y-Ts[I])>EPSILON ? Zs[I] : 0.0);
        return vec(In->xs(), Ys);
    }
};

QVector<ptr<operation> > availableOperations() {
#define mkop(Type) ptr<operation>(new op_##Type)
    return vec(mkop(diff), mkop(int), mkop(sum), mkop(neg), mkop(peaks), mkop(amplify), mkop(filter));
#undef mkop
}

//wrapper for curveItemModel, which it deletes itself
class curveItem : public QStandardItem {
public:
    curveItem(QStandardItem *Parent, curve* C);
    curve* getCurve() {return Curve;}
private:
    curve* Curve;
};

curveItem::curveItem(QStandardItem *Parent, curve* C) {
    Curve = C;
    this->setText(Curve->name());

    this->setColumnCount(2);

    if (C->isGroup()) {
        QStyle *Style = QApplication::style();
        this->setIcon(Style->standardIcon(QStyle::SP_DirIcon));
    } else {
        QPixmap Pixmap(32,32);
        Pixmap.fill(C->color());
        this->setIcon(QIcon(Pixmap));
        this->setCheckable(true);
        this->setCheckState(Qt::Checked);
    }
}

#define indexToCurve(Index) (((curveItem*)CurveListModel->itemFromIndex(Index))->getCurve())


curveItemModel::curveItemModel(QObject *Parent) : QStandardItemModel(Parent) {
}

QVariant curveItemModel::data(const QModelIndex &Index, int Role) {
    //if (Role == Qt::SizeHintRole) return QSize(16,16);
    return QStandardItemModel::data(Index, Role);
}

//wrapper for QwtPlotCurve, which it deletes itself
class curveSeriesWrapper : public QwtSeriesData<QPointF> {
public:
    curveSeriesWrapper(curve* C) {Curve = C;}

    size_t size() const {return Curve->size();}
    QPointF sample(size_t I) const {return QPointF(Curve->xs()[I], Curve->ys()[I]);}
    QRectF boundingRect() const {
        return QRectF(Curve->xs()[0], Curve->min(), Curve->xs()[size()-1], Curve->max());
    }
    //void setRectOfInterest(const QRectF &rect);

private:
    curve* Curve;
};

curveEditor::curveEditor(ptr<operation> Op, curveItemModel *Model, QWidget *Parent)
    : QDialog(Parent), Op(Op), CurveListModel(Model)
{
    let(MainLayout, new QVBoxLayout);

    this->setWindowTitle(Op->name());

    InputCurvesLayout = new QFormLayout;

    AddInputCurveButton = new QPushButton(trn("Add Curve"));
    connect(AddInputCurveButton, SIGNAL(clicked()), this, SLOT(addInputCurve()));
    InputCurvesLayout->addRow(new QLabel(""), AddInputCurveButton);

    let(ButtonsLayout, new QHBoxLayout);
    ButtonsLayout->addStretch();

    OkButton = new QPushButton(trn("&OK"));
    connect(OkButton, SIGNAL(clicked()), this, SLOT(ok()));
    OkButton->setEnabled(Op->arity() == 0);
    ButtonsLayout->addWidget(OkButton);

    let(CancelButton, new QPushButton(trn("Ca&ncel")));
    connect(CancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
    ButtonsLayout->addWidget(CancelButton);

    QGroupBox *InputCurvesGroup = new QGroupBox(trn("Source Curves"));
    InputCurvesGroup->setLayout(InputCurvesLayout);
    //InputCurvesGroup->setFlat(true);

    let(InputScalarsLayout, new QVBoxLayout);
    each(A, Op->args()) {
        ArgsCurves[A] = Op->argCurve(A);
        QString Value = Op->argValue(A);
        QString Default = Op->argDefault(A);
        let(Layout, new QFormLayout);

        ArgsComboBoxes[A] = new QComboBox;
        ArgsComboBoxes[A]->setEnabled(false);
        ArgsLineEdits[A] = new QLineEdit;
        ArgsLineEdits[A]->setEnabled(false);
        ArgsCurvesButtons[A] = new QPushButton(trn("Choose") + "...");
        ArgsCurvesButtons[A]->setEnabled(false);
        ArgsCurvesButtons[A]->setObjectName(A);
        if (ArgsCurves[A]) ArgsCurvesButtons[A]->setText(ArgsCurves[A]->name());

        if (Default == "<curve>") {
            ArgsCurvesButtons[A]->setEnabled(true);
            connect(ArgsCurvesButtons[A], SIGNAL(clicked()), this, SLOT(pickArgumentCurve()));
            Layout->addRow(new QLabel(trn("Value") + ":"), ArgsCurvesButtons[A]);
        } else if (Default.size() > 0 && Default[0] == '~') {
            ArgsComboBoxes[A]->setEnabled(true);
            QStringList Fields = Default.remove(0,1).split("|");
            Layout->addRow(new QLabel(trn("Value") + ":"), ArgsComboBoxes[A]);
            each(Field, Fields) ArgsComboBoxes[A]->addItem(trn(qPrintable(Field)));
            int Index = indexOf(X, Fields, X==Value);
            if (Index >= 0) ArgsComboBoxes[A]->setCurrentIndex(Index);
        } else {
            int AllowCurvesValues = AllowCurvesValues = Op->argAllowsCurve(A);
            ArgsLineEdits[A]->setText(Value);

            let(Radios, new QVBoxLayout);

            let(SourceRadioUser, new QRadioButton(trn("User Specified")));
            SourceRadioUser->setObjectName(A+",user_specified");
            connect(SourceRadioUser, SIGNAL(toggled(bool)), this, SLOT(switchValueSource(bool)));
            Radios->addWidget(SourceRadioUser);

            let(SourceRadioValues, new QRadioButton(trn("Curve's Values")));
            SourceRadioValues->setObjectName(A+",curve_values");
            connect(SourceRadioValues, SIGNAL(toggled(bool)), this, SLOT(switchValueSource(bool)));
            if (AllowCurvesValues) Radios->addWidget(SourceRadioValues);

            let(SourceRadioProperty, new QRadioButton(trn("Curve's Property")));
            SourceRadioProperty->setObjectName(A+",curve_property");
            connect(SourceRadioProperty, SIGNAL(toggled(bool)), this, SLOT(switchValueSource(bool)));
            Radios->addWidget(SourceRadioProperty);

            if (ArgsCurves[A]) {
                ArgsCurvesButtons[A]->setEnabled(true);
                ArgsCurvesButtons[A]->setText(ArgsCurves[A]->name());
                each(X, ArgsCurves[A]->properties())
                  ArgsComboBoxes[A]->addItem(ArgsCurves[A]->propertyName(X));
                QString P = Op->argCurveProperty(A);
                if (P == "<values>") {
                    SourceRadioValues->setChecked(true);
                } else {
                    SourceRadioProperty->setChecked(true);
                    ArgsComboBoxes[A]->setEnabled(true);
                    int Index = indexOf(X, ArgsCurves[A]->properties(), X==P);
                    if (Index >= 0) ArgsComboBoxes[A]->setCurrentIndex(Index);
                }
            } else {
                SourceRadioUser->setChecked(true);
                ArgsLineEdits[A]->setEnabled(true);
                QRegExp RegExp("[0123456789]*[.][0123456789]*");
                ArgsLineEdits[A]->setValidator(new QRegExpValidator(RegExp));

                //ArgsLineEdits[A]->setInputMask("0.0000");
                //ArgsLineEdits[A]->setMaxLength(6);
            }

            Layout->addRow(new QLabel(trn("Source of value") + ":"), Radios);
            Layout->addRow(new QLabel(trn("Value") + ":"), ArgsLineEdits[A]);
            Layout->addRow(new QLabel(trn("Curve") + ":"), ArgsCurvesButtons[A]);
            Layout->addRow(new QLabel(trn("Property") + ":"), ArgsComboBoxes[A]);
            connect(ArgsCurvesButtons[A], SIGNAL(clicked()), this, SLOT(pickPropertyCurve()));
        }
        let(G, new QGroupBox(Op->argName(A)));
        G->setLayout(Layout);
        InputScalarsLayout->addWidget(G);
    }

    //MainLayout->addWidget(new QLabel(Op->name() + ":"));
    MainLayout->addWidget(InputCurvesGroup);
    MainLayout->addLayout(InputScalarsLayout);
    MainLayout->addStretch();
    MainLayout->addLayout(ButtonsLayout);

    this->setLayout(MainLayout);

    OkButton->setEnabled(Op->arity() <= Curves.size());
    each (C, Op->operands()) addInputCurve(C);
}

void curveEditor::ok() {
    each(A, Op->args()) {
        QString Default = Op->argDefault(A);
        if (Default == "<curve>") {
            Op->setArgCurve(A,ArgsCurves[A]);
        } else if (Default.size() > 0 && Default[0] == '~') {
            QStringList Fields = Default.remove(0,1).split("|");
            QString F = Fields.at(ArgsComboBoxes[A]->currentIndex());
            Op->setArgValue(A, F);
        } else {
            if (ArgsLineEdits[A]->isEnabled()) {
                Op->setArgValue(A, ArgsLineEdits[A]->text());
                Op->setArgCurve(A,0);
            } else if (ArgsComboBoxes[A]->isEnabled()) {
                curve* C = ArgsCurves[A];
                QString P = C->properties()[ArgsComboBoxes[A]->currentIndex()];
                Op->setArgValue(A, QString::number(C->propertyValue(P)));
                Op->setArgCurve(A, C);
                Op->setArgCurveProperty(A, P);
            } else {
                Op->setArgValue(A, "<values>");
                Op->setArgCurve(A, ArgsCurves[A]);
                Op->setArgCurveProperty(A, "<values>");
            }
        }
    }
    Op->setOperands(operands());
    accept();
}

void curveEditor::cancel() {
    reject();
}

void curveEditor::setOperands(curves Cs) {
    each(C,Cs) addInputCurve(C);
}

void curveEditor::switchValueSource(bool) {
    QStringList Fields = sender()->objectName().split(",");
    QString Target = Fields.at(0);
    QString Type = Fields.at(1);
    if (Type == "user_specified") {
        ArgsLineEdits[Target]->setEnabled(true);
        ArgsCurvesButtons[Target]->setEnabled(false);
        ArgsComboBoxes[Target]->setEnabled(false);
        OkButton->setEnabled(Op->arity() <= Curves.size());
    } else if (Type == "curve_values") {
        ArgsLineEdits[Target]->setEnabled(false);
        ArgsCurvesButtons[Target]->setEnabled(true);
        ArgsComboBoxes[Target]->setEnabled(false);
        OkButton->setEnabled(ArgsCurves[Target] != 0 && Op->arity() <= Curves.size());
    } else if (Type == "curve_property") {
        ArgsLineEdits[Target]->setEnabled(false);
        ArgsCurvesButtons[Target]->setEnabled(true);
        ArgsComboBoxes[Target]->setEnabled(ArgsCurves[Target] != 0);
        OkButton->setEnabled(ArgsCurves[Target] != 0 && Op->arity() <= Curves.size());
    } else {
        QMessageBox::information(0, "error", trn("curveEditor::switchValueSource: invalid Type=") + Type);
        abort();
    }
}

void curveEditor::pickArgumentCurve() {
    QString Target = sender()->objectName();
    itemPickerDialog Picker(CurveListModel, this);
    unless (Picker.exec()) return;
    let(PickedIndex, Picker.pickedIndex());
    unless (PickedIndex.isValid()) return;
    let(C, indexToCurve(PickedIndex));
    ArgsCurves[Target] = C;
    ArgsCurvesButtons[Target]->setText(C->name());
}

void curveEditor::pickPropertyCurve() {
    QString Target = sender()->objectName();
    itemPickerDialog Picker(CurveListModel, this);
    unless (Picker.exec()) return;
    let(PickedIndex, Picker.pickedIndex());
    unless (PickedIndex.isValid()) return;
    let(C, indexToCurve(PickedIndex));
    ArgsCurves[Target] = C;
    ArgsCurvesButtons[Target]->setText(ArgsCurves[Target]->name());
    let(CB, ArgsComboBoxes[Target]);
    times(I, CB->count()) CB->removeItem(I);
    each(P, C->properties()) CB->addItem(C->propertyName(P));
    CB->setEnabled(true);
    OkButton->setEnabled(Op->arity() <= Curves.size());
}

void curveEditor::addInputCurve(curve *C) {
    unless (C) {
        itemPickerDialog Picker(CurveListModel, this);
        unless (Picker.exec()) return;
        let(PickedIndex, Picker.pickedIndex());
        unless (PickedIndex.isValid()) return;
        C = indexToCurve(PickedIndex);
    }

    let(L, new QLabel(C->name()));
    let(B, new QPushButton(trn("Change")));
    B->setObjectName(QString::number(Buttons.size()));
    connect(B, SIGNAL(clicked()), this, SLOT(changeCurve()));
    InputCurvesLayout->insertRow(Buttons.size(), L, B);

    ButtonsLabels.append(L);
    Buttons.append(B);
    Curves.append(C);

    AddInputCurveButton->setEnabled(Curves.size() < Op->arityMax());
    OkButton->setEnabled(Op->arity() <= Curves.size());
}

void curveEditor::changeCurve() {
    int Target = sender()->objectName().toInt();
    itemPickerDialog Picker(CurveListModel, this);
    unless (Picker.exec()) return;
    let(PickedIndex, Picker.pickedIndex());
    unless (PickedIndex.isValid()) return;
    let(C, indexToCurve(PickedIndex));
    ButtonsLabels[Target]->setText(C->name());
    Curves[Target] = C;
}


curve::curve(ptr<operation> Op) {
    doInit(Op);
}

curve::curve() {
    doInit(make_op(constant));
}

curve::~curve() {
    this->detach();
}


static struct {int Style, R, G, B;} PlotColors[] = {
    {0, 230,  60,  60},
    {0,  60, 230,  60},
    {0,  60,  60, 230},
    {0, 155, 230,   0},
    {0,   0, 155, 230},
    {0, 255,   0, 155},
    {0,  83,  78,  86},
    {0,   0,   0,   0}
};
static QColor indexToColor(int Id) {
    int I = Id%6;
    int R = PlotColors[I].R;
    int G = PlotColors[I].G;
    int B = PlotColors[I].B;
    return QColor(R,G,B);
}

QString operation::dynamicName() {
    let(Cs, this->operands());
    QString Name = this->type() + "[";
    times(I, Cs.size()) {
        if (I) Name += ",";
        Name += Cs[I]->name();
    }
    Name += "]";
    return Name;
}

void curve::doInit(ptr<operation> IOp) {
    static int NextCurveId = 1;
    Id = NextCurveId++;
    setOp(IOp);
    Name = Op->dynamicName();

    PlotCurve = make(QwtPlotCurve);
    setShow(1);

    PlotCurve->setStyle(QwtPlotCurve::Lines);
    //PlotCurve->setCurveAttribute(QwtPlotCurve::Fitted); //gives glitches on dense curves
    PlotCurve->setTitle(Name);
    PlotCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    PlotCurve->setSamples(new curveSeriesWrapper(this));
    //PlotCurve->setSymbol(new QwtSymbol(QwtSymbol::Cross, Qt::NoBrush,
    //        QPen(Qt::black), QSize(5, 5)));
    //QwtSymbol *Symbol = new QwtSymbol( QwtSymbol::Ellipse,
    //      QBrush( Qt::yellow ), QPen( Qt::red, 2 ), QSize( 8, 8 ) );
    //PlotCurve->setSymbol( Symbol );

    //for(i = 0; i < CurvCnt; i++)
    //  d_curves[i].setRawSamples( xval, yval, Size);

    setColor(indexToColor(Id-1));
}

void curve::update() {
    QVector<flts> Zs = Op->apply();
    Xs = Zs[0];
    Ys = Zs[1];

    // recalc aggregates...
    Min = Max = Average = Ys[0];
    MinAbs = MaxAbs = AverageAbs = abs(Ys[0]);
    each (Y, Ys) {
        if (Y < Min) Min = Y;
        if (Y > Max) Max = Y;
        Average += Y;
        double A = abs(Y);
        if (A < MinAbs) MinAbs = A;
        if (A > MaxAbs) MaxAbs = A;
        AverageAbs += A;
    }
    Average /= Ys.size();
    AverageAbs /= Ys.size();

    each(K, kids()) K->update();
}

void curve::setOp(ptr<operation> NewOp) {
    if (Op) {
        each(O, Op->operands()) O->removeKid(this);
        each(A, Op->args()) if (Op->argCurve(A)) Op->argCurve(A)->removeKid(this);
    }
    Op = NewOp;
    each(O, Op->operands()) O->addKid(this);
    each(A, Op->args()) if (Op->argCurve(A)) Op->argCurve(A)->addKid(this,false);
    update();
}

curves curve::parents() {
    return op()->operands();
}

bool curve::isGroup() {
    return op()->type()=="group";
}

bool curve::isDependent() {
    QString Type = op()->type();
    return Type != "file" && Type != "constant" && Type != "generate";
}

QVector<QString> curve::properties() {
    return vec(QString("min"), "max", "average", "minAbs", "maxAbs", "averageAbs");
}

QString curve::propertyName(QString Id) {
    if (Id == "min") return trn("Minimum");
    if (Id == "max") return trn("Maximum");
    if (Id == "average") return trn("Average");
    if (Id == "minAbs") return trn("Minimum Absolute");
    if (Id == "maxAbs") return trn("Maximum Absolute");
    if (Id == "averageAbs") return trn("Average Absolute");
    QMessageBox::information(0, "error", trn("curve::propertyName: invalid Id=") + Id);
    abort();
    return "";
}

double curve::propertyValue(QString Id) {
    if (Id == "min") return min();
    if (Id == "max") return max();
    if (Id == "average") return average();
    if (Id == "minAbs") return minAbs();
    if (Id == "maxAbs") return maxAbs();
    if (Id == "averageAbs") return averageAbs();
    QMessageBox::information(0, "error", trn("curve::propertyValue: invalid Id=") + Id);
    abort();
    return 0.0;
}

void curve::setShow(int State) {
    Show = State;
    if (PlotCurve) {
        if (Show) PlotCurve->setStyle(QwtPlotCurve::Lines);
        else PlotCurve->setStyle(QwtPlotCurve::NoCurve);
    }
}

void curve::setColor(QColor NewColor) {
    Color = NewColor;
    QPen Pen;
    Pen.setColor(Color);
    Pen.setWidthF(2);
    if (PlotCurve) PlotCurve->setPen(Pen);
}

void curve::attach(plot *Plot) {
    if (PlotCurve) PlotCurve->attach(Plot->qwtPlot());
}

void curve::detach() {
    if (PlotCurve) PlotCurve->detach();
}

void curve::addToGroup(curve* G) {
    if (any(X,Groups, X->id()==G->id())) return;
    Groups.append(G);
}

void curve::addKid(curve* C, bool Public) {
    if (any(K,Kids, C->id()==K->id())) return;
    Kids.append(C);
    if (Public) PublicKids.append(C);
}

void curve::removeKid(curve* C) {
    Kids = keep(K,Kids, C->id()!=K->id());
    PublicKids = keep(K,PublicKids, C->id()!=K->id());
}


operationPicker::operationPicker(QVector< ptr<operation> > Ops, QWidget *Parent)
    : QDialog(Parent), Ops(Ops), PickedOp(0)
{
    View = new QListWidget;
    View->setEditTriggers(QAbstractItemView::NoEditTriggers);
    View->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    connect(View, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(ok()));

    each(O, Ops) View->addItem(O->name());
    View->setCurrentRow(0);

    let(MainLayout, new QVBoxLayout);

    let(ButtonsLayout, new QHBoxLayout);
    ButtonsLayout->addStretch();

    let(PickButton, new QPushButton(trn("&OK")));
    connect(PickButton, SIGNAL(clicked()), this, SLOT(ok()));
    ButtonsLayout->addWidget(PickButton);

    let(CancelButton, new QPushButton(trn("Ca&ncel")));
    connect(CancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
    ButtonsLayout->addWidget(CancelButton);

    MainLayout->addWidget(View);
    MainLayout->addLayout(ButtonsLayout);

    this->setLayout(MainLayout);
}

void operationPicker::ok() {
    PickedOp = Ops[View->currentRow()];
    accept();
}

void operationPicker::cancel() {
    reject();
}


itemPickerDialog::itemPickerDialog(curveItemModel *M, QWidget *Parent, int PickWhat)
    : QDialog(Parent)
{
    CurvesAllowed = PickWhat&PICK_CURVE;
    GroupsAllowed = PickWhat&PICK_GROUP;
    CurveListModel = M;
    CurveListModel->setEditable(false);
    View = new QTreeView;
    View->setModel(CurveListModel);
    View->setEditTriggers(QAbstractItemView::NoEditTriggers);
    View->header()->hide();
    View->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    View->header()->setResizeMode(QHeaderView::ResizeToContents);
    connect(View, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(ok()));

    let(SelModel, View->selectionModel());
    connect(SelModel, SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&))
           ,this    , SLOT  (curvePicked      (const QModelIndex&, const QModelIndex&)));

    let(MainLayout, new QVBoxLayout);

    let(ButtonsLayout, new QHBoxLayout);
    ButtonsLayout->addStretch();

    OkButton = new QPushButton(trn("&OK"));
    connect(OkButton, SIGNAL(clicked()), this, SLOT(ok()));
    OkButton->setEnabled(false);
    ButtonsLayout->addWidget(OkButton);

    let(CancelButton, new QPushButton(trn("Ca&ncel")));
    connect(CancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
    ButtonsLayout->addWidget(CancelButton);

    MainLayout->addWidget(View);
    MainLayout->addLayout(ButtonsLayout);

    this->setLayout(MainLayout);
}

itemPickerDialog::~itemPickerDialog() {
    CurveListModel->setEditable(true);
}

void itemPickerDialog::curvePicked(const QModelIndex&, const QModelIndex&) {
    let(Index, View->currentIndex());
    Index = Index.sibling(Index.row(), 0);
    if (Index.isValid()) {
        curve* Row = indexToCurve(Index);
        if ((Row->isGroup() && GroupsAllowed) || (!Row->isGroup() && CurvesAllowed)) {
            OkButton->setEnabled(true);
            PickedIndex = Index;
            return;
        }
    }
    OkButton->setEnabled(false);
    PickedIndex = QModelIndex();
}

void itemPickerDialog::ok() {
    curvePicked(View->currentIndex(), View->currentIndex());
    if (PickedIndex.isValid()) accept();
}

void itemPickerDialog::cancel() {
    reject();
}


customTabBar::customTabBar(QWidget *Parent) : QTabBar(Parent) {
}

void customTabBar::mouseDoubleClickEvent(QMouseEvent *E) {
    if (E->button() != Qt::LeftButton) {
        QTabBar::mouseDoubleClickEvent (E);
        return;
    }
    int Index = currentIndex();
    bool Ok = true;
    QString NewName = QInputDialog::getText(
        this, trn("Rename"),
        trn("Enter New Name"),
        QLineEdit::Normal,
        tabText(Index),
        &Ok);

    if (Ok) {
        setTabText(Index, NewName);
        Q_EMIT textChanged(Index);
    }
}

tabWidget::tabWidget(QWidget *Parent) : QTabWidget(Parent) {
    TabBar = new customTabBar;
    setTabBar(TabBar);
    connect(TabBar, SIGNAL(textChanged(int)), this, SIGNAL(textChanged(int)));
    NewTabButton = new QToolButton(this);
    NewTabButton->setCursor(Qt::ArrowCursor);
    NewTabButton->setAutoRaise(true);
    this->setCornerWidget(NewTabButton, Qt::TopLeftCorner);
    NewTabButton->setIcon(QIcon(":/images/addtab.png"));
    QObject::connect(NewTabButton, SIGNAL(clicked()), this, SIGNAL(newTabRequested()));
    NewTabButton->setToolTip(trn("Add tab"));
    NewTabButton->hide();
}

void tabWidget::showNewTabButton(bool State) {
    if (State) NewTabButton->show();
    else NewTabButton->hide();
}


plot::plot(QWidget *Parent) : QWidget(Parent) {
    Plot = new QwtPlot;
    //XSlider = new QxtSpanSlider(Qt::Horizontal);
    //YSlider = new QxtSpanSlider(Qt::Vertical);
    Plot->setCanvasBackground(Qt::white);
    //Plot->insertLegend(new QwtLegend());

    int FW = 64;
    int FH = 32;
    XMin = new QLineEdit("0.0");
    YMin = new QLineEdit("0.0");
    XMax = new QLineEdit("1.0");
    YMax = new QLineEdit("1.0");
    XMin->setMaximumSize(FW,FH);
    YMin->setMaximumSize(FW,FH);
    XMax->setMaximumSize(FW,FH);
    YMax->setMaximumSize(FW,FH);
#if 0
    XSlider->setSpan(0,100);
    XSlider->setTickPosition(QSlider::TicksBelow);
    YSlider->setSpan(0,100);
    YSlider->setTickPosition(QSlider::TicksRight);
    //XSlider->setMinimum(0);
    //YSlider->setMinimum(0);
    //XSlider->setMaximum(100);
    //YSlider->setMaximum(100);

    //XSlider->setInvertedAppearance(true);
    //YSlider->setInvertedAppearance(true);
    //XSlider->setInvertedControls(true);
    //YSlider->setInvertedControls(true);
#endif
    //Plot->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(Plot, SIGNAL(customContextMenuRequested(const QPoint&))
           ,this, SIGNAL(customContextMenuRequested(const QPoint&)));

    QwtPlotGrid *Grid = new QwtPlotGrid();
    Grid->attach(Plot);

    Plot->setAxisScale(QwtPlot::xBottom, 0.0, 1.0);
    //Plot->setAxisTitle(QwtPlot::xBottom, trn("Frequency"));
    Plot->setAxisScale(QwtPlot::yLeft, 0.0, 1.0);
    //Plot->setAxisTitle(QwtPlot::yLeft, trn("Amplitude"));

    Zoomer = new QwtPlotZoomer(Plot->canvas());
    Zoomer->setRubberBandPen(QColor(Qt::black));
    Zoomer->setTrackerPen(QColor(Qt::black));
    Zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    Zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);

    Panner = new QwtPlotPanner(Plot->canvas());
    Panner->setMouseButton(Qt::MidButton);

    Plot->setMinimumSize(600,300); //sanitize width and height
    //Plot->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    setObjectName("<plot>");

#if 0
    QHBoxLayout *HBar = new QHBoxLayout;
    HBar->addWidget(XMin);
    HBar->addWidget(XSlider);
    HBar->addWidget(XMax);
    HBar->setSpacing(0);
    HBar->setMargin(0);
    QVBoxLayout *VBar = new QVBoxLayout;
    VBar->addWidget(YMax);
    VBar->addWidget(YSlider);
    VBar->addWidget(YMin);
    VBar->setSpacing(0);
    VBar->setMargin(0);
#endif

    QGridLayout *Layout = new QGridLayout;
    Layout->addWidget(Plot, 1, 1);
    //Layout->addLayout(HBar, 0, 1);
    //Layout->addLayout(VBar, 1, 0);
    //Layout->addWidget(XSlider, 0, 1);
    //Layout->addWidget(YSlider, 1, 0);
    setLayout(Layout);
}

void plot::setScale(double MinX, double MaxX, double MinY, double MaxY) {
    // should we translate Origin to Offset inside of curves?
    //int N = min(PlotSize, 500); // points in a viewport
    Plot->setAxisScale(QwtPlot::xBottom, MinX, MaxX);
    Plot->setAxisScale(QwtPlot::yLeft, MinY, MaxY);

    // re-zoom plot only if it wasn't user-zoomed previously
    QRectF ZR = Zoomer->zoomRect();
    int ZRI = Zoomer->zoomRectIndex();
    Zoomer->setZoomBase(); //make sure zoomer knows about new axis scales
    if (ZRI > 0) Zoomer->zoom(ZR);
}

void plot::replot() {
    Plot->replot();
}

projectInfoTab::projectInfoTab() {
    this->setColumnCount(1);
    this->horizontalHeader()->hide();
    this->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    this->verticalHeader()->setDefaultSectionSize(this->fontMetrics().height() + 2);
}

void projectInfoTab::addRow(QString Name, QString Value) {
    //this->setItem(Count, 0, new QTableWidgetItem(Name));
    if (Count >= this->rowCount()) this->setRowCount(Count+1);
    this->setVerticalHeaderItem(Count, new QTableWidgetItem(Name));
    this->setItem(Count, 0, new QTableWidgetItem(Value));
    Count++;
}

void projectInfoTab::updateInfo(projectView *P) {
    Count = 0;
    this->clear();
    this->setRowCount(0);
    if (!P) return;

    addRow(trn("Name"), P->getName());
    addRow(trn("File"), P->getFilename());
    addRow(trn("Description"), P->getDescription());
    addRow(trn("Source files") + ":", "");
    each(C, P->getCurves()) {
        if (C->isDependent()) continue;
        addRow(C->name(), C->source());
    }
}

curveInfoTab::curveInfoTab() {
    this->setColumnCount(1);
    this->horizontalHeader()->hide();
    this->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    this->verticalHeader()->setDefaultSectionSize(this->fontMetrics().height() + 2);
}

void curveInfoTab::addRow(QString Name, QString Value) {
    //this->setItem(Count, 0, new QTableWidgetItem(Name));
    if (Count >= this->rowCount()) this->setRowCount(Count+1);
    this->setVerticalHeaderItem(Count, new QTableWidgetItem(Name));
    this->setItem(Count, 0, new QTableWidgetItem(Value));
    Count++;
}

void curveInfoTab::updateInfo(curve* C) {
    Count = 0;
    this->clear();
    this->setRowCount(0);
    if (!C) return;
    if (C->isGroup()) return;

    QString Src = C->source();
    let(Ps, C->parents());
    if (Src == "") Src = trn("<generator>");
    Ps = keep(P, Ps, !P->isGroup());
    if (Ps.size()) {
        Src = "";
        times (I, Ps.size()) {
            if (I) Src += ", ";
            Src += Ps[I]->name();
        }
    }

    addRow(trn("Name"), C->name());
    addRow(trn("Source"), Src);
    addRow(trn("Channel"), C->channel());
    addRow(trn("Description"), C->description());
    addRow("", "");
    each(P, C->properties()) {
        addRow(C->propertyName(P), QString::number(C->propertyValue(P)));
    }
}


class curveItemDelegate : public QItemDelegate {
public:
  curveItemDelegate() {}
#if 0
  QSize sizeHint (const QStyleOptionViewItem & option, const QModelIndex & index) const
  {
    return QSize(100,16);
  }
#endif
};

projectView::projectView(QWidget *Parent) : QWidget(Parent) {
    Plot = 0;
    CurveCount = 0;
    PlotCount = 0;
    GroupCount = 0;

    CurveListModel = new curveItemModel(this);
    CurveListModel->setColumnCount(2);
    connect(CurveListModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&))
           ,this, SLOT(curveItemChanged(const QModelIndex&, const QModelIndex&)));
    CurveListView = new QTreeView;
    CurveListView->setModel(CurveListModel);
    //CurveListView->setItemDelegate(new curveItemDelegate());
    //PlotList->setSelectionMode(QAbstractItemView::MultiSelection);
    CurveListView->header()->hide();
    //PlotList->setMinimumSize(200/2,600/2);
    CurveListView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
    //PlotList->header()->resizeSection(0, 200);
    CurveListView->header()->setResizeMode(QHeaderView::ResizeToContents);
    //CurveListView->header()->setDefaultSectionSize(CurveListView->fontMetrics().height() + 2);
    //CurveListView->resizeColumnsToContents();
    CurveListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(CurveListView, SIGNAL(customContextMenuRequested(const QPoint&))
           ,this         , SLOT  (showCurveListContextMenu(const QPoint&)));
    let(SelModel, CurveListView->selectionModel());
    connect(SelModel, SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&))
           ,this    , SLOT  (curvePicked      (const QModelIndex&, const QModelIndex&)));
    let(Indices, CurveListView->selectionModel()->selectedRows());

    PlotTabs = new tabWidget;
    PlotTabs->setTabsClosable(true);
    PlotTabs->setDocumentMode(true);
    PlotTabs->showNewTabButton(true);

    connect(PlotTabs, SIGNAL(currentChanged(int)), this, SLOT(plotTabChanged(int)));
    connect(PlotTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(plotTabClose(int)));
    connect(PlotTabs, SIGNAL(newTabRequested()), this, SLOT(newPlot()));

    let(CurvesViewButtons, new QHBoxLayout);

    let(ImportButton, new QPushButton(trn("Add Curve from File")));
    connect(ImportButton, SIGNAL(clicked()), this, SLOT(import()));
    CurvesViewButtons->addWidget(ImportButton);

    let(AddGroupButton, new QPushButton(trn("Add Group")));
    connect(AddGroupButton, SIGNAL(clicked()), this, SLOT(newGroup()));
    CurvesViewButtons->addWidget(AddGroupButton);

    let(AddDependentButton, new QPushButton(trn("Add Dependent")));
    connect(AddDependentButton, SIGNAL(clicked()), this, SLOT(addDependentCurveFromNothing()));
    CurvesViewButtons->addWidget(AddDependentButton);

    let(GenerateButton, new QPushButton(trn("Generate")));
    connect(GenerateButton, SIGNAL(clicked()), this, SLOT(generate()));
    CurvesViewButtons->addWidget(GenerateButton);

    CurvesViewButtons->addStretch();

    let(PlotButtons, new QHBoxLayout);

    let(AddPlotButton, new QPushButton(trn("Add Plot")));
    connect(AddPlotButton, SIGNAL(clicked()), this, SLOT(newPlot()));
    PlotButtons->addWidget(AddPlotButton);

    QPushButton *DuplicatePlotButton = new QPushButton(trn("Duplicate"));
    //connect(DuplicatePlotButton, SIGNAL(clicked()), this, SLOT(duplicatePlot()));
    DuplicatePlotButton->setEnabled(false);
    PlotButtons->addWidget(DuplicatePlotButton);

    PlotButtons->addStretch();

    ProjectInfoTab = new projectInfoTab;
    ProjectInfoTab->setObjectName("<project_info_tab>");
    PlotTabs->addTab(ProjectInfoTab, QString(trn("Project")));
    PlotTabs->setCurrentWidget(ProjectInfoTab);

    CurveInfoTab = new curveInfoTab;
    CurveInfoTab->setObjectName("<curve_info_tab>");
    PlotTabs->addTab(CurveInfoTab, QString(trn("Curve")));
    PlotTabs->setCurrentWidget(CurveInfoTab);

#if 0
    //PlotTabs->tabBar()->tabButton(0, QTabBar::RightSide)->hide();
    //PlotTabs->tabBar()->tabButton(0, QTabBar::RightSide)->setMaximumSize(1,1);
    //PlotTabs->tabBar()->tabButton(1, QTabBar::RightSide)->hide();
    //PlotTabs->tabBar()->tabButton(1, QTabBar::RightSide)->setMaximumSize(1,1);
#endif

    newPlot();

    let(MainLayout, new QVBoxLayout);
    MainLayout->setSpacing(0); // margin between contained widgets
    MainLayout->setMargin(0); // margin with neighbor widgets
    MainLayout->addLayout(CurvesViewButtons);
    MainLayout->addWidget(CurveListView);
    MainLayout->addWidget(PlotTabs);
    MainLayout->addLayout(PlotButtons);

    setLayout(MainLayout);

    replot();
}

projectView::~projectView() {
}

curve *projectView::addDependentCurveFromCurves(curves Cs) {
    operationPicker OpPicker(availableOperations(), this);
    unless (OpPicker.exec()) return 0;
    let(Op, OpPicker.pickedOp());
    curveEditor CurveEditor(Op, CurveListModel, this);
    CurveEditor.setOperands(Cs);
    unless (CurveEditor.exec()) return 0;

    let(Args, CurveEditor.operands());
    curve* C = make_curve(Op);
    C->setShow(1);
    C->attach(Plot);
    addCurve(C);
    return C;
}

void projectView::addDependentCurveFromContext() {
    let(Index, CurveListView->currentIndex());
    Index = Index.sibling(Index.row(), 0);
    unless (Index.isValid()) return;
    addDependentCurveFromCurves(vec(indexToCurve(Index)));
}

void projectView::addDependentCurveFromSenderId() {
    int TargetId = sender()->objectName().toInt();
    int I = indexOf(C,Curves,C->id()==TargetId);
    if (I < 0) return;
    curve* C = Curves[I];
    addDependentCurveFromCurves(vec(C));
}

void projectView::addDependentCurveFromNothing() {
    addDependentCurveFromCurves(curves());
}

void projectView::curvePicked(const QModelIndex&, const QModelIndex&) {
    let(Index, CurveListView->currentIndex());
    Index = Index.sibling(Index.row(), 0);
    if (Index.isValid()) {
        curve* Row = indexToCurve(Index);
        if (!Row->isGroup()) {
            CurveInfoTab->updateInfo(Row);
            return;
        }
    }
    CurveInfoTab->updateInfo(0);
}

void projectView::replot() {
    double MinX = 0.000, MaxX = 1.000, MinY = 0.000, MaxY = 1.000;
    each (C, Curves) {
        flts Xs = C->xs();
        //flts Ys = C->ys();
        MinX = min(MinX, Xs[0]);
        MaxX = max(MaxX, Xs[Xs.size()-1]);
        MinY = min(MinY, C->min());
        MaxY = max(MaxY, C->max());
    }
    Plot->setScale(MinX, MaxX, MinY, MaxY);
    Plot->replot();
}


void projectView::removeCurveById(int TargetId, bool UpdateList) {
    int I = indexOf(C,Curves,C->id()==TargetId);
    if (I < 0) return;
    curve* C = Curves[I];
    C->setRemoved(true); //TODO: garbage collect it
    let(Kids, C->kids());
    let(Parents, C->parents());
    each (K, Kids) removeCurveById(K->id());
    each (P, C->parents()) P->removeKid(C);
    C->detach();
    if (UpdateList) updateCurveList();
    //Curves.remove(I);
    replot();
}

void projectView::removeCurve() {
    int TargetId = sender()->objectName().toInt();
    removeCurveById(TargetId);
}

void projectView::addCurve(curve* C) {
    unless (indexOf(X,Curves,X->id()==C->id()) > -1) Curves.append(C);
    C->setRemoved(false);
    updateCurveList();
}

void projectView::addCurveUnder(curve *C, QModelIndex ParentIndex) {
    let(Parent, CurveListModel->invisibleRootItem());
    if (ParentIndex.isValid()) Parent = CurveListModel->itemFromIndex(ParentIndex);

    QStandardItem *Item = new curveItem(Parent, C);

    Parent->appendRow(Item);
    QModelIndex Index = CurveListModel->index(Parent->rowCount()-1, 0, Parent->index());
    C->addModelIndex(Index);

    if (C->isGroup()) {
        Item->setCheckable(false);
    } else {
        Item->setCheckable(true);
        Item->setCheckState(C->show() ? Qt::Checked : Qt::Unchecked);
        Index = Item->index().sibling(Item->row(), 1);

        QToolButton *DependentCurveButton = new QToolButton(this);
        DependentCurveButton->setCursor(Qt::ArrowCursor);
        DependentCurveButton->setAutoRaise(true);
        DependentCurveButton->setIcon(QIcon(":/images/addtab.png"));
        DependentCurveButton->setToolTip(trn("Create Dependent Curve"));
        DependentCurveButton->setObjectName(QString::number(C->id()));
        DependentCurveButton->setMaximumSize(30,24);
        QObject::connect(DependentCurveButton, SIGNAL(clicked())
                        ,this, SLOT(addDependentCurveFromSenderId()));

        let(W, new QWidget);
        QHBoxLayout *L = new QHBoxLayout;
        L->addWidget(DependentCurveButton);
        L->addStretch();
        L->setMargin(0);
        L->setSpacing(0);
        W->setLayout(L);
        CurveListView->setIndexWidget(Index, W);
    }
}

void projectView::curveItemChanged(const QModelIndex &Index, const QModelIndex &/*BottomRight*/) {
    QModelIndex ItemIndex = Index.sibling(Index.row(), 0);
    if (ItemIndex.isValid()) {
        curveItem *Item = (curveItem*)CurveListModel->itemFromIndex(ItemIndex);
        curve* C = indexToCurve(ItemIndex);
        C->setName(Item->text());
        unless (C->isGroup()) {
            C->setShow(Item->checkState() == Qt::Checked);
            //if (ItemIndex == CurveListView->currentIndex())
                CurveInfoTab->updateInfo(C);
        }
    }
    model_foreach(Index,QModelIndex(), CurveListModel, ({ //propagate change to other items
         curveItem *Item = (curveItem*)CurveListModel->itemFromIndex(Index);
         curve* C = Item->getCurve();
         Item->setText(C->name());
         unless (C->isGroup()) {
             Item->setCheckState(C->show() ? Qt::Checked : Qt::Unchecked);
         }
    }));
    replot();
}

void projectView::updateCurveList() {
    // this method handles curve list view updates, when curves being added/removed
    bool NeedsReplot = false;
    QHash<int, QVector<QModelIndex> > PM; //parent map
    if (0) {
again:
        NeedsReplot = true;
    }
    model_foreach(Index,QModelIndex(), CurveListModel, ({ // remove unlinked curves
         curveItem *Item = (curveItem*)CurveListModel->itemFromIndex(Index);
         curve* C = Item->getCurve();
         curves Ps = C->parents() + C->groups();
         QStandardItem *ParentItem = Item->parent();
         if (ParentItem && ParentItem != CurveListModel->invisibleRootItem()) {
            curve *ParentCurve = ((curveItem*)ParentItem)->getCurve();
            QModelIndex Ind = ParentItem->index();
            if (!any(X,PM[C->id()],X==Ind)) PM[C->id()].append(ParentItem->index());
            if (C->removed() || all(P,Ps, P->id()!=ParentCurve->id())) {
                ParentItem->removeRows(Item->row(), 1);
                C->removeModelIndex(Index);
                goto again;
            }
         } else if (C->removed()) {
             CurveListModel->invisibleRootItem()->removeRows(Item->row(), 1);
             C->removeModelIndex(Index);
             goto again;
         }
    }));

    each(C, Curves) { // add new curves
        if (C->removed()) continue;
        curves Ps = C->parents() + C->groups();
        if (C->parents().size() < 1 && C->modelIndices().size() < 1) { //Root is the parent
            addCurveUnder(C, QModelIndex());
            C->attach(Plot);
            goto again;
        }
        each (P, Ps) {
            each(Ind, P->modelIndices()) {
                if (any(X,PM[C->id()],X==Ind)) continue;
                PM[C->id()].append(Ind);
                if (PM[C->id()].size() == 1) C->attach(Plot);
                addCurveUnder(C, Ind);
                goto again;
            }
        }
    }
    replot();
}

void projectView::newGroup() {
    let(G, make_curve(make_op(group)));
    G->setName(trn("group%1").arg(GroupCount++));
    addCurve(G);
}

void projectView::showCurveInfo() {
     PlotTabs->setCurrentWidget(CurveInfoTab);
}

void curveItemModel::setEditable(bool State) {
    model_foreach(Index,QModelIndex(), this, ({
        curveItem *Item = (curveItem*)this->itemFromIndex(Index);
        if (State) {
            Item->setFlags(Item->flags() | Qt::ItemIsEditable);
            unless (Item->getCurve()->isGroup()) Item->setCheckable(true);
        } else {
            Item->setFlags(Item->flags() & ~Qt::ItemIsEditable);
            unless (Item->getCurve()->isGroup()) Item->setCheckable(false);
        }
    }));
}

void projectView::showCurveListContextMenu(const QPoint&) {
    //let(Index, PlotList->indexAt(P));
    let(Index, CurveListView->currentIndex());
    Index = Index.sibling(Index.row(), 0);
    unless (Index.isValid()) return;

    curve* C = indexToCurve(Index);
    int Id = C->id();
    QString StrId = QString::number(Id);

    let(Menu, make(QMenu));
    QAction *A;
    if (C->isGroup()) {
        A = Menu->addAction(trn("Rename"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(renameItem()));

        A = Menu->addAction(trn("Create Curve under this Group"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(createCurveUnderGroup()));

        A = Menu->addAction(trn("Create Subgroup"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(addSubgroupToGroup()));

        A = Menu->addAction(trn("Add Group Below"));
        A->setObjectName(StrId);
        A->setEnabled(0);
        //connect(A, SIGNAL(triggered()), this, SLOT(addGroupBelowToGroup()));

        A = Menu->addAction(trn("Add Group Above"));
        A->setObjectName(StrId);
        A->setEnabled(0);
        //connect(A, SIGNAL(triggered()), this, SLOT(addGroupAboveToGroup()));

        Menu->addSeparator();

        A = Menu->addAction(trn("Remove Group"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(removeCurve()));
    } else {
        A = Menu->addAction(trn("Rename"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(renameItem()));

        if (C->isDependent()) {
            A = Menu->addAction(trn("Obtaining Method"));
            A->setObjectName(StrId);
            connect(A, SIGNAL(triggered()), this, SLOT(editCurve()));
        } else {
            A = Menu->addAction(trn("Load from file"));
            A->setObjectName(StrId);
            connect(A, SIGNAL(triggered()), this, SLOT(reloadCurveFromFile()));
        }

        A = Menu->addAction(trn("Description and Aggregate Info"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(showCurveInfo()));

        A = Menu->addAction(trn("Create Dependent Curve"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(addDependentCurveFromContext()));

        A = Menu->addAction(QString(C->show() ? "[x] " : "[ ] ") + trn("Add to the Current Plot"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(toggleCurvePlotVisibility()));

        A = Menu->addAction(trn("Create Processing Module from the Curve Production Algorithm"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(createProcessingModule()));

        A = Menu->addAction(trn("Move Curve to Group"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(moveCurveToGroup()));

        Menu->addSeparator();

        A = Menu->addAction(trn("Remove Curve"));
        A->setObjectName(StrId);
        connect(A, SIGNAL(triggered()), this, SLOT(removeCurve()));
    }

    //QPoint GP = PlotList->mapToGlobal(P);
    Menu->exec(QCursor::pos());
}

#define senderAsCurve() ({ \
    int TargetId = sender()->objectName().toInt(); \
    int I = indexOf(C,Curves,C->id()==TargetId); \
    if (I < 0) return; \
    Curves[I]; \
    })

void projectView::renameItem() {
    curve* C = senderAsCurve();
    bool Ok = true;
    QString NewName = QInputDialog::getText(
                this, trn("Rename"),
                trn("Enter New Name"),
                QLineEdit::Normal,
                C->name(),
                &Ok);
    if (!Ok) return;
    CurveListModel->itemFromIndex(C->modelIndices()[0])->setText(NewName);
}

void projectView::createCurveUnderGroup() {
    curve* G = senderAsCurve();
    curve* C = addDependentCurveFromCurves(curves());
    unless (C) return;
    C->addToGroup(G);
    updateCurveList();
}


void projectView::addSubgroupToGroup() {
    let(Index, CurveListView->currentIndex());
    Index = Index.sibling(Index.row(), 0);
    unless (Index.isValid()) return;
    curve *Parent = indexToCurve(Index);
    unless (Parent->isGroup()) return; //no groups under a curve
    let(G, make_curve(make_op(group)));
    G->op()->setOperands(vec(Parent));
    G->setName(trn("group%1").arg(GroupCount++));
    addCurve(G);
}

void projectView::toggleCurvePlotVisibility() {
    curve* C = senderAsCurve();
    CurveListModel->itemFromIndex(C->modelIndices()[0])->setCheckState(C->show() ? Qt::Unchecked : Qt::Checked);
}

static curves depsOf(curve *C) {
    curves As = C->op()->operands() + map(A,C->op()->args(), C->op()->argCurve(A));
    curves Bs;
    each(A,As) Bs += depsOf(A);
    return As + Bs;
}

// добавляет в Model элементы родительских кривых от C
void depPickerDialog::populateTree(QStandardItem *Parent, curve *C) {
    QStandardItem *Item = new curveItem(Parent, C);
    Parent->appendRow(Item);
    Item->setCheckable(true);
    Item->setCheckState(Qt::Checked);
    View->expand(Item->index());
    //Item->setCheckState(C->show() ? Qt::Checked : Qt::Unchecked);
    curves Xs = C->op()->operands() + map(A,C->op()->args(), C->op()->argCurve(A));
    each (X,Xs) populateTree(Item, X);
}


depPickerDialog::depPickerDialog(curve *C, curves Cs, QWidget *Parent)
    : QDialog(Parent)
{
    Model = new curveItemModel(this);
    //Model->setEditable(false);
    Model->setColumnCount(2);
    connect(Model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&))
           ,this , SLOT(curveItemChanged(const QModelIndex&, const QModelIndex&)));
    View = new QTreeView;
    View->setModel(Model);
    View->setEditTriggers(QAbstractItemView::NoEditTriggers);
    View->header()->hide();
    View->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    View->header()->setResizeMode(QHeaderView::ResizeToContents);

    //connect(View, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(ok()));

    let(SelModel, View->selectionModel());
    connect(SelModel, SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&))
           ,this    , SLOT  (curvePicked      (const QModelIndex&, const QModelIndex&)));

    let(MainLayout, new QVBoxLayout);

    let(ButtonsLayout, new QHBoxLayout);
    ButtonsLayout->addStretch();

    OkButton = new QPushButton(trn("&OK"));
    connect(OkButton, SIGNAL(clicked()), this, SLOT(ok()));
    OkButton->setEnabled(false);
    ButtonsLayout->addWidget(OkButton);

    let(CancelButton, new QPushButton(trn("Ca&ncel")));
    connect(CancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
    ButtonsLayout->addWidget(CancelButton);

    MainLayout->addWidget(View);
    MainLayout->addLayout(ButtonsLayout);

    this->setLayout(MainLayout);

    populateTree(Model->invisibleRootItem(), C);
    //each(X,depsOf(C)) dbg("%s", qPrintable(X->name()));
}

curves collectAllOffsprings(curveItemModel *Model, const QModelIndex BaseIndex) {
    curves Rs;
    model_foreach(Index,BaseIndex, Model, ({ //propagate change to other items
        curveItem *Item = (curveItem*)Model->itemFromIndex(Index);
        curve* C = Item->getCurve();
        Rs.append(C);
    }));
    return Rs;
}

void depPickerDialog::curveItemChanged(const QModelIndex &Index, const QModelIndex &/*BottomRight*/) {
#if 0
    QModelIndex ItemIndex = Index.sibling(Index.row(), 0);
    if (ItemIndex.isValid()) {
        curveItem *Item = (curveItem*)CurveListModel->itemFromIndex(ItemIndex);
        curve* C = indexToCurve(ItemIndex);
        C->setName(Item->text());
        unless (C->isGroup()) {
            C->setShow(Item->checkState() == Qt::Checked);
            //if (ItemIndex == CurveListView->currentIndex())
                CurveInfoTab->updateInfo(C);
        }
    }
    model_foreach(Index,QModelIndex(), Model, ({ //propagate change to other items
         curveItem *Item = (curveItem*)CurveListModel->itemFromIndex(Index);
         curve* C = Item->getCurve();
         Item->setText(C->name());
         unless (C->isGroup()) {
             Item->setCheckState(C->show() ? Qt::Checked : Qt::Unchecked);
         }
    }));
#endif
}

void depPickerDialog::curvePicked(const QModelIndex&, const QModelIndex&) {
}

void depPickerDialog::ok() {
    //curvePicked(View->currentIndex(), View->currentIndex());
    //if (PickedIndex.isValid())
    accept();
}

void depPickerDialog::cancel() {
    reject();
}

void projectView::createProcessingModule() {
    curve* C = senderAsCurve();
    depPickerDialog Picker(C,Curves,this);
    unless (Picker.exec()) return;

#define saveEach(X,Xs,Body) do { \
    int GENSYM(I) = 0; \
    if (Xs.size() == 0) S << "<none>"; \
    else each(X,Xs) { \
        if (GENSYM(I)++) S << ";"; \
        S << ({Body;}); \
    } \
    S << "\n"; \
} while(0)
    let(FileName, QFileDialog::getSaveFileName
                    (0
                     , trn("Save File")
                     , QDir::currentPath()
                     , "PMF files (*.pmf);;Text files (*.txt);;All files (*.*)"
                     , new QString("PMF files (*.pmf)")));
    if (FileName == "") return;
    QFile F(FileName);
    unless (F.open(QIODevice::WriteOnly)) {
        QMessageBox::information(0, "error", F.errorString());
        return;
    }

    QTextStream S(&F);

    //times (I, Xs.size()) S << Xs[I] << "," << Ys[I] << "\n";

    F.close();
#undef saveEach
}

void projectView::editCurve() {
    curve* C = senderAsCurve();
    curveEditor CurveEditor(C->op(), CurveListModel, this);
    unless (CurveEditor.exec()) return;
    C->update();
    replot();
}

void projectView::reloadCurveFromFile() {
    let(FileName, QFileDialog::getOpenFileName
      ( 0
      , trn("Import Curve")
      , QDir::currentPath() + "/samples"
      //, QDir::currentPath()
      , "AFC files (*.afc);;CSV files (*.csv);;Text files (*.txt);;All files (*.*)"
      , new QString("CSV files (*.csv)")));
    if (FileName == "") return;
    curve* C = senderAsCurve();
    C->setOp(make_op(file,FileName));
    C->setSource(FileName);
    replot();
}

void projectView::moveCurveToGroup() {
    curve* C = senderAsCurve();
    itemPickerDialog Picker(CurveListModel, this, PICK_GROUP);
    unless (Picker.exec()) return;
    let(PickedIndex, Picker.pickedIndex());
    unless (PickedIndex.isValid()) return;

    curve *G = indexToCurve(PickedIndex);

    if (any(X,C->groups(), X->id()==G->id())) {
        QMessageBox::information(0, "error", trn("Already Contains"));
        return;
    }

    C->addToGroup(G);
    updateCurveList();
}

void projectView::import() {
    let(FileName, QFileDialog::getOpenFileName
      ( 0
      , trn("Import Curve")
      , QDir::currentPath() + "/samples"
      //, QDir::currentPath()
      , "AFC files (*.afc);;CSV files (*.csv);;Text files (*.txt);;All files (*.*)"
      , new QString("CSV files (*.csv)")));
    if (FileName == "") return;

    let(C, make_curve(make_op(file,FileName)));
    C->setName(QFileInfo(FileName).baseName());
    C->setSource(FileName);
    addCurve(C);
}

void projectView::generate() {
    let(C, make_curve(make_op(generate,"sin", 100)));
    C->setName(trn("curve%1").arg(CurveCount++));
    addCurve(C);
}

void projectView::save() {
    let(FileName, QFileDialog::getSaveFileName
                    (0
                     , trn("Save File")
                     , QDir::currentPath()
                     , "CSV files (*.csv);;Text files (*.txt);;All files (*.*)"
                     , new QString("CSV files (*.csv)")));
    if (FileName == "") return;
    QFile F(FileName);
    unless (F.open(QIODevice::WriteOnly)) {
        QMessageBox::information(0, "error", F.errorString());
        return;
    }

    QTextStream S(&F);
    //times (I, Xs.size()) S << Xs[I] << "," << Ys[I] << "\n";
    F.close();
}

void projectView::newPlot(bool MakeCurrent) {
    let(NewPlot, new plot);
    if (!Plot) Plot = NewPlot;

    NewPlot->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(NewPlot, SIGNAL(customContextMenuRequested(const QPoint&))
           ,this   , SLOT  (showPlotContextMenu(const QPoint&)));

    //NewPlot->setTitle(trn("Spectrum"));
    QSizePolicy SizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    SizePolicy.setHeightForWidth(true);
    NewPlot->setSizePolicy(SizePolicy);
    PlotTabs->addTab(NewPlot, QString(trn("plot%1")).arg(PlotCount++));
    if (MakeCurrent) PlotTabs->setCurrentWidget(NewPlot);
}

void projectView::showPlotContextMenu(const QPoint&) {
    //plot *P =  (plot*)PlotTabs->currentWidget();

    let(Menu, make(QMenu));
    QAction *A;

    A = Menu->addAction(trn("Rename"));
    A->setEnabled(0);
    //connect(A, SIGNAL(triggered()), this, SLOT(renameCurve()));

    Menu->addSeparator();

    A = Menu->addAction(trn("Remove"));
    connect(A, SIGNAL(triggered()), this, SLOT(removePlot()));

    Menu->exec(QCursor::pos());
}


void projectView::removePlot() {
    plotTabClose(PlotTabs->currentIndex());
}

void projectView::syncPlotAttachedCurves() {
    QHash<int,int> AttachedCurves;
    each (C, Curves) {
        if (C->show()) AttachedCurves[C->id()] = 1;
    }
    Plot->setAttachedCurves(AttachedCurves);
}

void projectView::plotTabChanged(int Index) {
    let(W, PlotTabs->widget(Index));
    if (W->objectName() != "<plot>") {
        if (W->objectName() == "<project_info_tab>") {
            ((projectInfoTab*)W)->updateInfo(this);
        }
        return;
    }
    syncPlotAttachedCurves();
    Plot = (plot*)PlotTabs->widget(Index);
    QHash<int,int> AttachedCurves = Plot->attachedCurves();

    model_foreach(Index,QModelIndex(), CurveListModel, ({
         curveItem *Item = (curveItem*)CurveListModel->itemFromIndex(Index);
         curve* ItemCurve = Item->getCurve();
         each (C, Curves) {
             if (C->id() != ItemCurve->id()) continue;
             if (AttachedCurves[C->id()]) {
                 Item->setCheckState(Qt::Checked);
                 C->setShow(1);
                 C->attach(Plot);
             } else {
                 Item->setCheckState(Qt::Unchecked);
                 C->setShow(0);
                 C->attach(Plot);
             }
         }
    }));

    Plot->replot();
}

void projectView::plotTabClose(int Index) {
    if (PlotTabs->widget(Index)->objectName() != "<plot>") {
        return; // avoid removing info tabs
    }
    PlotTabs->removeTab(Index);
    // FIXME: to simplify implementation, we dont delete the underlaying page widget,
    //        so unnoticeable memory leak occurs here
}


// replace it with method in PlotTabs
#define INFO_TABS_COUNT 2

#define saveEach(X,Xs,Body) do { \
    int GENSYM(I) = 0; \
    if (Xs.size() == 0) S << "<none>"; \
    else each(X,Xs) { \
        if (GENSYM(I)++) S << ";"; \
        S << ({Body;}); \
    } \
    S << "\n"; \
} while(0)
void projectView::saveProject() {
    QFile F(getFilename());
    unless (F.open(QIODevice::WriteOnly)) {
        QMessageBox::information(0, "error", F.errorString());
        return;
    }
    QTextStream S(&F);

    //FIXME: save format version, in case we want to support several versions

    S << this->getName() << "\n";
    saveEach(C,Curves,C->id());

    let(TabNames, map(I,seq(INFO_TABS_COUNT, PlotTabs->count()), PlotTabs->tabText(I)));

    saveEach(T,TabNames,T);

    // for each plot tab, save which curves it displays
    syncPlotAttachedCurves();
    each(Index,seq(INFO_TABS_COUNT, PlotTabs->count())) {
        plot *P = (plot*)PlotTabs->widget(Index);
        let(AC, P->attachedCurves());
        let(Ids, map(C,Curves,C->id()));
        saveEach(Id, keep(Id,Ids,AC[Id]), Id);
    }

    saveEach(C,Curves,C->name());
    saveEach(C,Curves,C->source());
    saveEach(C,Curves,C->channel());
    saveEach(C,Curves,C->description());
    saveEach(C,Curves,QString("%1,%2,%3").arg(C->color().red())
                                         .arg(C->color().green())
                                         .arg(C->color().blue()));
    saveEach(C,Curves,C->size());
    saveEach(C,Curves,C->op()->type());

    each(C,Curves) {
        saveEach(G,C->groups(),G->id());
        let(Op, C->op());
        saveEach(C,Op->operands(),C->id());
        saveEach(A,Op->args(),A);
        saveEach(A,Op->args(),Op->argValue(A));
        saveEach(A,Op->args(),Op->argCurveProperty(A));
        saveEach(A,Op->args(),Op->argCurve(A) ? Op->argCurve(A)->id() : 0);
        saveEach(V,C->xs(),V);
        saveEach(V,C->ys(),V);
    }

    F.close();
}
#undef saveEach


//no idea why, but some GCC versions require force it's instantiation
//even using it doesn't help
template class QVector<ptr<operation> >;

#define loadList() ({ \
    QString L = S.readLine(); \
    L == "<none>" ? QList<QString>() : L.split(";"); \
})
bool projectView::initFromFile() {
    int I, J;
    QHash<int,int> AddedCurves;

    QFile F(getFilename());
    unless (F.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(0, "error", F.errorString());
        return false;
    }
    QTextStream S(&F);

    QString ProjectName = S.readLine();
    this->setName(ProjectName);

    let(SIds, loadList());
    let(PlotNames, map(X,loadList(),X));

    while (PlotTabs->count() > INFO_TABS_COUNT) PlotTabs->removeTab(INFO_TABS_COUNT);
    each(N, PlotNames) { // tabs must exist, before we create curves
        newPlot(PlotTabs->currentIndex() != INFO_TABS_COUNT);
        PlotTabs->setTabText(PlotTabs->currentIndex(), N);
    }

    curves Curves;
    QHash<int,curve*> OldToNew;
    // make dummy curve for each of the previous curves
    each(OldId, map(SId, SIds, SId.toInt())) {
        let(C, make_curve(make_op(constant)));
        // establish correspondence between old and new id
        // because we cant reuse the old id, as it may already be occupied
        OldToNew[OldId] = C;
        Curves.append(C);
    }
    times (I, PlotNames.size()) {
        QHash<int,int> AttachedCurves;
        each(Id,loadList()) AttachedCurves[OldToNew[Id.toInt()]->id()] = 1;
        plot *P = (plot*)PlotTabs->widget(I+INFO_TABS_COUNT);
        P->setAttachedCurves(AttachedCurves);
        if (I==0) each(C,Curves) C->setShow(AttachedCurves[C->id()]);
    }

    let(CurveNames, loadList());

    let(CurveSources, loadList());
    let(CurveChannels, loadList());
    let(CurveDescriptions, loadList());
    let(CurveColors, loadList());

    I = 0;
    each(C, Curves) {
        C->setName(CurveNames[I]);
        C->setSource(CurveSources[I]);
        C->setChannel(CurveChannels[I]);
        C->setDescription(CurveDescriptions[I]);
        let(Zs, map(Z,CurveColors[I].split(","), Z.toInt()));
        C->setColor(QColor(Zs[0], Zs[1], Zs[2]));
        I++;
    }

    loadList(); // skip curve sizes

    let(OpTypes, loadList());

    I = 0;
    each(C,Curves) {
        each(X,loadList()) C->addToGroup(OldToNew[X.toInt()]);

        let(Operands, map(X,loadList(), keep(C,Curves,C->id()==OldToNew[X.toInt()]->id())[0]));
        let(Args, map(X,loadList(),X));
        let(ArgValues, map(X,loadList(),X));
        let(ArgCurveProperties, map(X,loadList(),X));
        let(ArgCurves, map(X,loadList(),OldToNew[X.toInt()]));
        let(Xs, map(X,loadList(),X.toDouble()));
        let(Ys, map(X,loadList(),X.toDouble()));

        let(As,keep(A,availableOperations(), A->type()==OpTypes[I]));
        if (OpTypes[I] == "file" || OpTypes[I] == "constant" || OpTypes[I] == "generate") {
            As = vec(make_op(constant, Xs, Ys));
        } else if (OpTypes[I] == "group") {
            As = vec(make_op(group));
        } else if (As.size() != 1) {
            QMessageBox::information(0, trn("error"), trn("Missing processing module: ") + OpTypes[I]);
            goto fail;
        }
        ptr<operation> O = As[0];

        O->setOperands(Operands);

        J = 0;
        each(A,Args) {
            O->setArgCurve(A, ArgCurves[J]);
            O->setArgValue(A, ArgValues[J]);
            O->setArgCurveProperty(A, ArgCurveProperties[J]);
            J++;
        }
        C->setOp(O);
        I++;
    }

    F.close();

    // build curve tree
    while (AddedCurves.size() < Curves.size()) {
        each(C,Curves) {
            addCurve(C);
            AddedCurves[C->id()] = 1;
        }
    }

    replot();
    return true;

fail:
    F.close();
    return false;
}
#undef loadList


void projectView::saveProjectAs() {
    let(FileName, QFileDialog::getSaveFileName
                    (0
                     , trn("Save Project")
                     , QDir::currentPath() + "/saves"
                     , "Text files (*.txt);;All files (*.*)"
                     , new QString("Text files (*.txt)")));
    if (FileName == "") return;
    if (QFileInfo(FileName).suffix() != "txt") FileName += ".txt";
    setFilename(FileName);
    saveProject();
}


mainWindow::mainWindow(QWidget *Parent) : QMainWindow(Parent) {
    Project = 0;
    ProjectCount = 0;
    setWindowTitle(trn("Curve Analyzer"));

    //let(MenuBar, new QMenuBar(this)); //required for OSX
    let(MenuBar, this->menuBar());
    let(FileMenu, MenuBar->addMenu(trn("&File")));
    let(NewProjectAction, FileMenu->addAction(trn("New Project")));
    connect(NewProjectAction, SIGNAL(triggered()), this, SLOT(newProject()));
    let(OpenProjectAction, FileMenu->addAction(trn("Open Project...")));
    connect(OpenProjectAction, SIGNAL(triggered()), this, SLOT(openProject()));
    let(SaveAction, FileMenu->addAction(trn("Save Project")));
    connect(SaveAction, SIGNAL(triggered()), this, SLOT(saveProject()));
    let(SaveProjectAsAction, FileMenu->addAction(trn("Save Project As...")));
    connect(SaveProjectAsAction, SIGNAL(triggered()), this, SLOT(saveProjectAs()));
    FileMenu->addSeparator();
    let(ExitAction, FileMenu->addAction(trn("E&xit"))); //Note: Qt4.8 on MacOSX autohides Exit button
    connect(ExitAction, SIGNAL(triggered()), this, SLOT(close()));
    //MenuBar->addMenu(FileMenu);
    //MainLayout->setMenuBar(MenuBar); //could be useful

    let(ProjectMenu, MenuBar->addMenu(trn("Project")));
    let(NewPlotAction, ProjectMenu->addAction(trn("New Plot")));
    //connect(NewPlotAction, SIGNAL(triggered()), this, SLOT(newPlot()))
    let(RemoveCurrentPlotAction, ProjectMenu->addAction(trn("Remove Current Plot")));
    //connect(RemoveCurrentPlotAction, SIGNAL(triggered()), this, SLOT(removeCurrentPlot()));
    let(LoadCurveFromFileAction, ProjectMenu->addAction(trn("Load Curve from File")));
    //connect(LoadCurveFromFileAction, SIGNAL(triggered()), this, SLOT(loadCurveFromFile()));

    let(InfoMenu, MenuBar->addMenu(trn("Aggregate Info")));
    let(ModulesMenu, MenuBar->addMenu(trn("Processing Modules")));

    let(LoadNewPorcessingModuleAction, ModulesMenu->addAction(trn("Load New Processing Module")));
    //connect(LoadNewPorcessingModuleAction, SIGNAL(triggered()), this, SLOT(newProcessingModule()));
    let(ListPorcessingModulesAction, ModulesMenu->addAction(trn("List Processing Modules")));
    //connect(ListPorcessingModulesAction, SIGNAL(triggered()), this, SLOT(listProcessingModules()));

    let(MainLayout, new QVBoxLayout);

    let(ButtonsLayout, new QHBoxLayout);
    //ButtonsLayout->addStretch();
    //let(ExitButton, new QPushButton(trn("E&xit")));
    //connect(ExitButton, SIGNAL(clicked()), ExitAction, SLOT(trigger()));
    //ButtonsLayout->addWidget(ExitButton);

    ProjectTabs = new tabWidget;
    ProjectTabs->setTabsClosable(true);
    ProjectTabs->setDocumentMode(true);
    ProjectTabs->showNewTabButton(true);
    connect(ProjectTabs, SIGNAL(currentChanged(int)), this, SLOT(projectTabChanged(int)));
    connect(ProjectTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(projectTabClose(int)));
    connect(ProjectTabs, SIGNAL(newTabRequested()), this, SLOT(newProject()));

    //setCentralWidget(ProjectTabs);

    let(CentralWidget, new QWidget);
    setCentralWidget(CentralWidget);
    CentralWidget->setLayout(MainLayout);

    MainLayout->addWidget(ProjectTabs);
    MainLayout->addLayout(ButtonsLayout);

    MainLayout->setSpacing(0); // margin between contained widgets
    //MainLayout->setMargin(0); // margin with neighbor widgets

    newProject();
    //projectView *PV = Project = new projectView;
    //ProjectTabs->addTab(PV, QString(trn("project%1")).arg(ProjectCount++));
}

mainWindow::~mainWindow() {
}


void mainWindow::newProject() {
    let(NewProject, new projectView);
    if (!Project) Project = NewProject;
    QSizePolicy SizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    SizePolicy.setHeightForWidth(true);
    NewProject->setSizePolicy(SizePolicy);
    NewProject->setName(trn("project%1").arg(ProjectCount++));
    ProjectTabs->addTab(NewProject, NewProject->getName());
    ProjectTabs->setCurrentWidget(NewProject);
    connect(ProjectTabs, SIGNAL(textChanged(int)), this, SLOT(projectTabRenamed(int)));
}

void mainWindow::removeProject() {
    projectTabClose(ProjectTabs->currentIndex());
}

void mainWindow::projectTabChanged(int Index) {
    Project = (projectView*)ProjectTabs->widget(Index);
}

void mainWindow::projectTabClose(int Index) {
    ProjectTabs->removeTab(Index);
    // FIXME: to simplify implementation, we dont delete the underlaying page widget,
    //        so unnoticeable memory leak occurs here
}

void mainWindow::projectTabRenamed(int Index) {
    ((projectView*)ProjectTabs->widget(Index))->setName(ProjectTabs->tabText(Index));
}

void mainWindow::openProject() {
    let(FileName, QFileDialog::getOpenFileName
        (0
        , trn("Import Curve")
        , QDir::currentPath() + "/saves"
        , "Text files (*.txt);;All files (*.*)"
        , new QString("Text files (*.txt)")));
    if (FileName == "") return;

    newProject();
    projectView *P = (projectView*)ProjectTabs->currentWidget();
    P->setFilename(FileName);
    unless (P->initFromFile()) {
        removeProject();
        return;
    }
    ProjectTabs->setTabText(ProjectTabs->currentIndex(), P->getName());
}

void mainWindow::saveProject() {
    if (Project) Project->saveProject();
}

void mainWindow::saveProjectAs() {
    if (Project) Project->saveProjectAs();
}









static struct {
    char const *Original;
    char const *Translated;
} Trns[] = {
    {"Add tab", "Добавить вкладку"},
    {"E&xit", "Вы&ход"},
    {"&Open", "&Открыть"},
    {"New Project", "Создать Новый Проект"},
    {"Open Project...", "Загрузить Проект..."},
    {"Open Project", "Загрузить Проект"},
    {"Save Project", "Сохранить Проект"},
    {"Save Project As...", "Сохранить Проект Как..."},
    {"&File", "&Фаил"},
    {"Project", "Проект"},
    {"New Plot", "Новый График"},
    {"Remove Current Plot", "Убрать Текущий График"},
    {"Load Curve from File", "Загрузить Кривую из Файла"},
    {"Aggregate Info", "Сводные Данные"},
    {"Processing Modules", "Модули Обработки"},
    {"Load New Processing Module", "Загрузить Новый Модуль Обработки"},
    {"List Processing Modules", "База модулей обработки"},
    {"Ca&ncel", "Отме&нить"},
    {"&OK", "&ОК"},
    {"Load", "Загрузить"},
    {"Remove", "Убрать"},
    {"Spectrum", "Спектр"},
    {"Curve Analyzer", "Анализатор Кривых"},
    {"Amplitude", "Амплитуда"},
    {"Frequency", "Частота"},
    {"Add Curve from File", "Добавить кривую из файла"},
    {"Add Group", "Добавить группу"},
    {"Add Dependent", "Добавить Зависимую"},
    {"Generate", "Сгенерировать"},
    {"Add Plot", "Добавить График"},
    {"Duplicate", "Дублировать"},
    {"plot%1", "график%1"},
    {"curve%1", "кривая%1"},
    {"project%1", "проект%1"},
    {"group%1", "группа%1"},
    {"Integrate", "Интегрировать"},
    {"Differentiate", "Дифференцировать"},
    {"Sum", "Сложить"},
    {"Negate", "Негатив"},
    {"Find Peaks", "Найти Пики"},
    {"Amplify", "Усилить"},
    {"Scale", "Масштаб"},
    {"Filter", "Фильтр"},
    {"Threshold", "Порог"},
    {"Comparison Type", "Тип Сравнения"},
    {"greater", "больше"},
    {"greater or equal", "больше или равно"},
    {"less", "меньше"},
    {"less or equal", "меньше или равно"},
    {"equal", "равно"},
    {"not equal", "не равно"},
    {"Result Type", "Тип Результата"},
    {"value/0", "значение/0"},
    {"Curve for result", "Кривая для результата"},
    {"Source", "Источник"},
    {"Name", "Имя"},
    {"File", "Фаил"},
    {"Value", "Значение"},
    {"Channel", "Канал"},
    {"Description", "Описание"},
    {"Minimum", "Минимум"},
    {"Maximum", "Максимум"},
    {"Average", "Среднее"},
    {"Minimum Absolute", "Минимум Модуля"},
    {"Maximum Absolute", "Максимум Модуля"},
    {"Average Absolute", "Среднее Модуля"},
    {"<generator>", "<генератор>"},
    {"Source files", "Исходные файлы"},
    {"Load from file", "Загрузить из файла"},
    {"Obtaining Method", "Метод получения"},
    {"Description and Aggregate Info", "Описание и сводные данные"},
    {"Create Dependent Curve", "Создать производную кривую"},
    {"Add to the Current Plot", "Добавить на текущий график"},
    {"Create Processing Module from the Curve Production Algorithm", "Создать модуль обработки из алгоритма получения кривой"},
    {"Move Curve to Group", "Перенести кривую в группу"},
    {"Remove Curve", "Удалить кривую"},
    {"Add to Group", "Добавить в группу"},
    {"Create Curve under this Group", "Создать в группе кривую"},
    {"Create Subgroup", "Создать подгруппу"},
    {"Add Group Below", "Добавить группу ниже"},
    {"Add Group Above", "Добавить группу выше"},
    {"Remove Group", "Удалить группу"},
    {"Already Contains", "Уже Содержит"},
    {"Rename", "Переименовать"},
    {"Enter New Name", "Введите Новое Имя"},
    {"Source Curves", "Исходные Кривые"},
    {"Add Curve", "Добавить Кривую"},
    {"Change", "Изменить"},
    {"Source of value", "Источник значения"},
    {"User Specified", "Пользовательское Значение"},
    {"Curve's Values", "Значения Кривой"},
    {"Curve's Property", "Свойство Кривой"},
    {"Curve", "Кривая"},
    {"Project", "Проект"},
    {"Choose", "Выбрать"},
    {"Property", "Свойство"},
    {0, 0}
};

QString trn(char const *M, char const *Context) {
    if (!Context) Context = "";
    for (int I = 0; Trns[I].Original; I++) {
        if (!strcmp(M, Trns[I].Original))
            return QString::fromUtf8(Trns[I].Translated);
    }
    return M;
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);


#if 0
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
            QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);
    // dbg("locale: %s", qPrintable(QLocale::system().name()));
    //dbg("%s", qPrintable(QLibraryInfo::location(QLibraryInfo::TranslationsPath)));

    QTranslator appTranslator;
    //appTranslator.load("trn_ru_RU");
    //appTranslator.load("trn_" + QLocale::system().name());
    appTranslator.load("trn_ru_RU", "/home/snv/prj/funfeats/src");
    a.installTranslator(&appTranslator);
#endif

    mainWindow w;
    w.show();

    return a.exec();
}
