// This is a public domain code; use at your own discretion
// Copyright (C) 2013   Nikita Vadimovich Sadkov

#ifndef COMMON_H
#define COMMON_H

#include <QVector>
#include <QString>
#include <QPointF>

/*
When you try to stringize something with #x, that x must be a macro parameter:
  #define FOO #__LINE__   - this is not okay
  #define BAR(x) #x       - this is okay

But you cannot simply say BAR(__LINE__), because this will pass the token __LINE__ into BAR, where it is immediately turned into a string without expansion (this is by design), giving "__LINE__". The same thing happens with the token-pasting operator ##: expansion of their operands never happens.

The solution is to add indirection. You should always have these in your codebase somewhere:*/
#define STRINGIZE(x) STRINGIZE_SIMPLE(x)
#define STRINGIZE_SIMPLE(x) #x
#define CONCAT(first, second) CONCAT_SIMPLE(first, second)
#define CONCAT_SIMPLE(first, second) first ## second

//debug print
#define dbg(Args...) (fprintf(stderr, Args), fprintf(stderr, "\n"))

#define trace dbg("Trace %s:%d:%s", __FILE__, __LINE__, __FUNCTION__);

//preprocessor magic
//#define __GENSYM2(x,y) x##y
//#define __GENSYM1(x,y) __GENSYM2(x,y)
//#define GENSYM(x) __GENSYM1(x,__COUNTER__)

// this is a bad gensym, because several same macros can be nested on a single line
// alas __COUNTER__ doesn't work with some compiler setups
#define GENSYM2(N,L) __##N##L
#define GENSYM1(N,L) GENSYM2(N,L)
#define GENSYM(N) GENSYM1(N,__LINE__)

#define let(Place,Value) __typeof__(Value) Place = (Value)
#define unless(x) if(!(x))
#define till(x) while(!(x))
#define times(I,N) for(int I=0, GENSYM(C)=(N); I<GENSYM(C); I++)
//#define times(I,N) for(int I=0; I<(N); I++)


template <class T> QVector<T> collectionAsVector(QList<T> Xs) {
    QVector<T> Ys;
    int Sz = Xs.size();
    for(int I = 0; I < Sz; I++) Ys.append(Xs[I]);
    return Ys;
}

template <class T> QVector<T> collectionAsVector(QVector<T> Xs) {
    return Xs;
}


#define each(X, Xs) \
  for (int GENSYM(_each_H)=1, GENSYM(_each_I)=0, GENSYM(_each_F)=0; GENSYM(_each_H); GENSYM(_each_H)=0) \
    for (let(GENSYM(_each_Xs), collectionAsVector(Xs)); GENSYM(_each_I)<GENSYM(_each_Xs).size();GENSYM(_each_I)++) \
      for (let(X, GENSYM(_each_Xs)[GENSYM(_each_I)]); (GENSYM(_each_F)=!GENSYM(_each_F));)

//#define each(X, Xs) foreach(__typeof__ (Xs[0]) X, Xs)


#define map(X,Xs,Body) ({ \
   __typeof__(Xs) GENSYM(_map_Xs) = (Xs); \
   __typeof__(GENSYM(_map_Xs)[0]) X; \
   __typeof__(({Body;})) GENSYM(_map_Y); \
   QVector<__typeof__(GENSYM(_map_Y))> GENSYM(_map_Ys); \
   int GENSYM(_map_K); \
   for(GENSYM(_map_K)=0; GENSYM(_map_K)<GENSYM(_map_Xs).size(); GENSYM(_map_K)++) { \
        X = GENSYM(_map_Xs)[GENSYM(_map_K)]; \
        GENSYM(_map_Ys).append(({Body;})); \
   } \
   GENSYM(_map_Ys); \
})

#define keep(X, Xs, F) ({ \
    __typeof__(Xs) GENSYM(R); \
    each(X, Xs) { \
        if (F) GENSYM(R).append(X); \
    } \
    GENSYM(R); \
})

#define all(X, Xs, F) ({ \
    bool GENSYM(R) = true; \
    for (int GENSYM(I)=0; GENSYM(I)<Xs.size(); GENSYM(I)++) { \
        let(&X, Xs[GENSYM(I)]); \
        unless (({F;})) { \
            GENSYM(R) = false; \
            goto GENSYM(_all_end); \
        } \
    } \
    GENSYM(_all_end): \
    GENSYM(R); \
})

#define any(X, Xs, F) ({ \
    bool GENSYM(R) = false; \
    for (int GENSYM(I)=0; GENSYM(I)<Xs.size(); GENSYM(I)++) { \
        let(&X, Xs[GENSYM(I)]); \
        if (({F;})) { \
            GENSYM(R) = true; \
            goto GENSYM(_any_end); \
        } \
    } \
    GENSYM(_any_end): \
    GENSYM(R); \
})

#define indexOf(X, Xs, F) ({ \
    int GENSYM(R) = -1; \
    for (int GENSYM(I)=0; GENSYM(I)<Xs.size(); GENSYM(I)++) { \
        let(&X, Xs[GENSYM(I)]); \
        if (({F;})) { \
            GENSYM(R) = GENSYM(I); \
            goto GENSYM(_indexOf_end); \
        } \
    } \
    GENSYM(_indexOf_end): \
    GENSYM(R); \
})


template <class T> class appender {
    QVector<T> &Xs;
public:
    appender(QVector<T> &IXs) : Xs(IXs) {}

    void append() {}
    void append(T A0) {
        append();
        Xs.append(A0);
    }
    void append(T A0, T A1) {
        append(A0);
        Xs.append(A1);
    }
    void append(T A0, T A1, T A2) {
        append(A0, A1);
        Xs.append(A2);
    }
    void append(T A0, T A1, T A2, T A3) {
        append(A0, A1, A2);
        Xs.append(A3);
    }
    void append(T A0, T A1, T A2, T A3, T A4) {
        append(A0, A1, A2, A3);
        Xs.append(A4);
    }
    void append(T A0, T A1, T A2, T A3, T A4, T A5) {
        append(A0, A1, A2, A3, A4);
        Xs.append(A5);
    }
    void append(T A0, T A1, T A2, T A3, T A4, T A5, T A6) {
        append(A0, A1, A2, A3, A4, A5);
        Xs.append(A6);
    }
};

#define vec(First, Args...) ({ \
    QVector<__typeof__(First)> GENSYM(Xs); \
    GENSYM(Xs).append(First); \
    appender<__typeof__(First)>(GENSYM(Xs)).append(Args); \
    GENSYM(Xs); \
})

#define ptr QSharedPointer
#define make(Type, Args...) ptr<Type>(new Type(Args))


typedef QVector<int> ints;
typedef QVector<double> flts;
typedef QVector<QString> strs;

// custom internationalization function
QString tr(char const *M);

// reinventing the wheel
template <class T> T abs(T V) {return V < 0 ? -V : V;}
template <class T> T min(T A, T B) {return A < B ? A : B;}
template <class T> T max(T A, T B) {return A > B ? A : B;}
template <class T> int sign(T V) {
    if (V > (T)0) return 1;
    if (V < (T)0) return -1;
    return 0;
}


//let(Xs, map(X, seq(1, 10), X*X));
//each(X,Xs) printf("%d\n", X);
inline ints seq(int Start, int End) {
    ints Xs;
    if (Start < End) {
        while (Start < End) Xs.append(Start++);
    } else {
        while (Start > End) Xs.append(Start--);
    }
    return Xs;
}


#define prop(Type, Name) \
    public: Type get##Name() {return Name;} \
            void set##Name(Type In##Name) {Name = In##Name;} \
    private: Type Name;

#define prop_bool(Name) \
    public: bool is##Name() {return Name;} \
            void set##Name(bool In##Name) {Name = In##Name;} \
    private: bool Name;

// read-only property
#define prop_r(Type, Name) \
    public: Type get##Name() {return Name;} \
    private: Type Name;

#endif // COMMON_H
