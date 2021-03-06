/**
* Filename: "emusetun.c "
*
* Project: Виртуальная машина МЦВМ "Сетунь" 1958 года на языке Си
*
* Create date: 01.11.2018
* Edit date:   11.02.2021
*
* Version: 1.24
*/

/**
 *  Заголовочные файла
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/** ******************************
 *  Виртуальная машина Сетунь-1958
 * -------------------------------
 */
#define TRI_TEST 	(0) 

/* Макросы максимальное значения тритов */ 
#define TRIT1_MAX	(+1)
#define TRIT1_MIN	(-1)
#define TRIT3_MAX	(+13)
#define TRIT3_MIN	(-13)
#define TRIT4_MAX	(+40)
#define TRIT4_MIN	(-40)
#define TRIT5_MAX	(+121)
#define TRIT5_MIN	(-121)
#define TRIT9_MAX	(+9841)
#define TRIT9_MIN	(-9841)
#define TRIT18_MAX	(+193710244L)
#define TRIT18_MIN	(-193710244L)

/* *******************************************
 * Реализация виртуальной машины "Сетунь-1958"
 * --------------------------------------------
 */

/*
 *  Описание тритов в троичном числе в поле бит 
 *
 *  A(1...5) 	A,tb = f4,f3,f2,f1,f0
 *  K(1...9)	K,tb = f4,f3,f2,f1,f0
 *  D(1...9)	D,tb = f8,f7,f6,f5,f4,f3,f2,f1,f0
 *  S(1...18)	S,tb = f17,...,f0
 *
 *  Поле бит    f1,f0 = b3,b2,b1,b0 		
 */

/**
 *  Троичные типы данных памяти "Сетунь-1958"
 */
#define SIZE_WORD_SHORT	  		(9)		/* короткое слово 9-трит  */
#define SIZE_WORD_LONG	  		(18)	/* длинное слово  18-трит */

/**
 * Описание ферритовой памяти FRAM
 */
#define NUMBER_ZONE_FRAM		(3)		/* количество зон ферритовой памяти */
#define SIZE_ZONE_TRIT_FRAM		(54)	/* количнество коротких 9-тритных слов в зоне */
#define SIZE_ALL_TRIT_FRAM		(162)	/* всего количество коротких 9-тритных слов */

#define SIZE_PAGES_FRAM 	    (2)	/* количнество коротких 9-тритных слов в зоне */
#define SIZE_PAGE_TRIT_FRAM 	(81)	/* количнество коротких 9-тритных слов в зоне */

/**
 * Адреса зон ферритовой памяти FRAM
 */
#define ZONE_M_FRAM_BEG  (-120)	/* ----0 */
#define ZONE_M_FRAM_END  (-41)  /* -++++ */
#define ZONE_0_FRAM_BEG  (-40)  /* 0---0 */
#define ZONE_0_FRAM_END  ( 40)  /* 0++++ */
#define ZONE_P_FRAM_BEG  ( 42)  /* +---0 */
#define ZONE_P_FRAM_END  (121)  /* +++++ */

/**
 * Описание магнитного барабана DRUM
 */
#define SIZE_TRIT_DRUM			(3888)	/* количество хранения коротких слов из 9-тритов */
#define SIZE_ZONE_TRIT_DRUM		(54)	/* количество 9-тритных слов в зоне */
#define NUMBER_ZONE_DRUM		(72)	/* количество зон на магнитном барабане */

/**
 * Тип данных троичного числа
 */
/**
 * Типы данных для виртуальной троичной машины "Сетунь-1958"
 */
typedef uint32_t trishort; 
typedef uint64_t trilong; 
typedef uintptr_t addr;

typedef struct trs { 
	uint8_t  l;			/* длина троичного числа в тритах			*/
	trilong tb; 		/* двоичное битовое поле троичного числа 	*/
} trs_t;


/**
 * Статус выполнения операции  "Сетунь-1958"
 */
enum {
	OK 			= 0,	/* Успешное выполнение операции */
	WORK 		= 1,	/* Выполнение операций виртуальной машины */ 
	END 		= 2,	/* TODO для чего ? */
	STOP_DONE 	= 3,	/* Успешный останов машины */	
	STOP_OVER 	= 4,	/* Останов по переполнению результата операции машины */
	STOP_ERROR 	= 5		/* Аварийный останов машины */	
};

/**
 * Определение памяти машины "Сетунь-1958"
 */ 
trishort mem_fram[SIZE_PAGE_TRIT_FRAM][SIZE_PAGES_FRAM]; /* оперативное запоминающее устройство на ферритовых сердечниках */
trishort mem_drum[NUMBER_ZONE_DRUM][SIZE_ZONE_TRIT_DRUM]; /* запоминающее устройство на магнитном барабане */

/** ***********************************
 *  Определение регистров "Сетунь-1958"
 *  -----------------------------------
 */
/* Основные регистры в порядке пульта управления */
trs_t K;  /* K(1:9)  код команды (адрес ячейки оперативной памяти) */
trs_t F;  /* F(1:5)  индекс регистр  */
trs_t C;  /* C(1:5)  программный счетчик  */
trs_t W;  /* W(1:1)  знак троичного числа */
//
trs_t S;  /* S(1:18) аккумулятор */
trs_t R;  /* R(1:18) регистр множителя */
trs_t MB; /* MB(1:4) троичное число зоны магнитного барабана */
/* Дополнительные */
trs_t MR; /* временный регистр для обмена троичным числом */

/** --------------------------------------------------
 *  Прототипы функций виртуальной машины "Сетунь-1958"
 *  --------------------------------------------------
 */
int32_t pow3( int8_t x); 
int8_t trit2bit(trs_t t);	
trs_t bit2trit(int8_t b);
int32_t trs_to_digit( trs_t *x );

/**
 * Операции с тритами
 */
void and_t(int8_t *a, int8_t *b, int8_t *s );
void xor_t(int8_t *a, int8_t *b, int8_t *s );
void or_t(int8_t *a, int8_t *b, int8_t *s );
void sum_t(int8_t *a, int8_t *b, int8_t *p0, int8_t *s, int8_t *p1 );

/**
 * Операции с полями тритов
 */
void clear(trs_t *t);

uint8_t get_trit( trs_t t, int8_t pos) ;
int8_t get_trit_int( trs_t t, int8_t pos) ;
void set_trit( trs_t *t, int8_t pos, int8_t v);
uint8_t bit2tb(int8_t b);

int8_t sgn(trs_t t);
int8_t inc_trs(trs_t *t);
int8_t dec_trs(trs_t *t);
trs_t shift_trs(trs_t t, int8_t s);
trs_t add_trs(trs_t a, trs_t b);
trs_t and_trs(trs_t a, trs_t b);
trs_t or_trs(trs_t a, trs_t b);
trs_t xor_trs(trs_t a, trs_t b);
trs_t sub_trs(trs_t a, trs_t b);
trs_t mul_trs(trs_t a, trs_t b);
trs_t div_trs(trs_t a, trs_t b);
trs_t slice_trs( trs_t t, int8_t p1, int8_t p2);
int32_t tb_to_digit( trishort tb );

/**
 * Определить следующий адрес
 */
trs_t next_address(trs_t c);  

/**
 * Преобразование тритов в другие типы данных
 */
uint8_t trit2lt(int8_t v);
int8_t symtrs2numb(uint8_t c); 
int8_t str_symtrs2numb(uint8_t * s); 
trs_t smtr(uint8_t * s);
void trit2linetape(trs_t v, uint8_t * lp);
uint8_t linetape2trit(uint8_t * lp, trs_t * v);

/**
 * Очистить памяти FRAM
 */
void clean_fram(void);

/**
 * Операции с ферритовой памятью машины
 */
trs_t ld_fram( trs_t ea );
void st_fram( trs_t ea, trs_t v );

/**
 * Очистить память магнитного барабана DRUM
 */
void clean_drum(void);

trs_t ld_drum( trs_t ea );
void st_drum( trs_t ea, trs_t v );

/**
 * Устройства структуры машины Сетунь-1958
 */
void reset_setun(void);						/* Сброс машины */
trs_t control_trs( trs_t a );				/* Устройство управления */
int8_t execute_trs(trs_t addr, trs_t oper);	/* Выполнение кодов операций */

/**
 * Печать отладочной информации
 */
void view_short_reg(trs_t *t, uint8_t *ch);
void view_short_regs(void);


/** ---------------------------------------------------
 *  Реализации функций виртуальной машины "Сетунь-1958"
 *  ---------------------------------------------------
 */

/** 
 * Возведение в степень по модулю 3
 */
int32_t pow3( int8_t x ) {
    int8_t i;
    int32_t r = 1;    
    for(i=0; i<x; i++) {
		r *= 3;
    }        
	return r;
}


/**
 * Преобразование int в битое поле f0 как [b1b0]
 */
uint8_t bit2tb(int8_t b) {	
	if (b > 0) {		
		return 2; /* +1 */
	}
	if (b < 0) {		
		return 1; /* -1 */
	}	
	/* b == 0 */
	return 0;
}

/**
 * Преобразование младшего трита в битое поле f0=[b1b0]
 */
int8_t trit2bit(trs_t t) {
	switch ( t.tb & (uint8_t)3 ) {
		case 0:				/* f0 = [b1b0] = [00] */
		case 3: return 0; 	/* f0 = [b1b0] = [11] */
			break;
		case 2: return 1; 	/* f0 = [b1b0] = [10] */
			break;
		case 1: return -1; 	/* f0 = [b1b0] = [01] */
			break;
	}
	return 0;
}

/**
 * Преобразование поля f0=[b1b0] трита в число со знаком
 */
int8_t tb2int(uint8_t tb) {
	switch ( tb & (uint8_t)3 ) {
		case 0:
		case 3: return 0; 
			break;
		case 2: return 1; 	
			break;
		case 1: return -1; 	
	}
	return 0;	
}

/**
 * Преобразование младшего трита в челое число битого поля
 */
uint8_t trit2low(trs_t t) {
	switch ( t.tb & (trilong)03 ) {
		case 0:
		case 3: return 0; break;
		case 2: return 2; break;
		case 1: return 1;break; 	
	}
	return 0;	
}

/**
 * Преобразование числа со знаком в битовое поле f0 в троичное число
 */
trs_t bit2trit(int8_t b) {
	trs_t r;	
	if (b > 0) {
		r.tb = 2;
		return r;
	}
	if (b < 0) {
		r.tb = 1;
		return r;
	}
	/* b == 0 */
	r.tb = 0;
	return r;		
}

/**
 * Очистить длину троичного числа и поле битов троичного числа
 */
void clear(trs_t *t) {	
	t->l  = 0;
	t->tb = 0;
}

/** 
 * Троичное сложение двух тритов с переносом
 */
void sum_t(int8_t *a, int8_t *b, int8_t *p0, int8_t *s, int8_t *p1 ) {
	switch (*a + *b + *p0)
	{     
		case -3: {*s =  0; *p1 = -1;} break;
		case -2: {*s =  1; *p1 = -1;} break;
		case -1: {*s = -1; *p1 =  0;} break;	
		case  0: {*s =  0; *p1 =  0;} break;
		case  1: {*s =  1; *p1 =  0;} break;
		case  2: {*s = -1; *p1 =  1;} break;
		case  3: {*s =  0; *p1 =  1;} break;
		default: {*s =  0; *p1 =  0;} break;	
	}
}

/**
 *  Троичное умножение тритов
 */
void and_t(int8_t *a, int8_t *b, int8_t *s ) {
	if( (*a * *b)>0 ) {
		*s = 1;
	}
	else if( (*a * *b)<0 ) {
		*s = -1;
	}
	else {
		*s = 0;
	}
}

/**
 * Троичное xor тритов
 */
void xor_t(int8_t *a, int8_t *b, int8_t *s ) {
	if( *a == -1 && *b == -1 ) {
		*s = 1;
	}
	else if( *a == 1 && *b == -1 ) {
		*s = 0;
	}
	else if( *a == -1 && *b == 1 ) {
		*s = 0;
	}
	else if( *a ==  1 && *b == 1 ) {
		*s = -1;
	}
	else if( *a ==  0 && *b == 1 ) {
		*s = -1;
	}
	else if( *a ==  0 && *b == -1 ) {
		*s = -1;
	}
	else if( *a ==  1 && *b == 0 ) {
		*s = 1;
	}
	else if( *a ==  -1 && *b == 0 ) {
		*s = -1;
	}
	else {
		*s = 0;
	}
}

/**
 * Троичное xor тритов
 */
void or_t(int8_t *a, int8_t *b, int8_t *s ) {
	if( *a == -1 && *b == -1 ) {
		*s = -1;
	}
	else if( *a == 1 && *b == -1 ) {
		*s = 1;
	}
	else if( *a == -1 && *b == 1 ) {
		*s = 1;
	}
	else if( *a ==  1 && *b == 1 ) {
		*s = 1;
	}
	else if( *a ==  0 && *b == 1 ) {
		*s = 1;
	}
	else if( *a ==  0 && *b == -1 ) {
		*s = 0;
	}
	else if( *a ==  1 && *b == 0 ) {
		*s = 1;
	}
	else if( *a ==  -1 && *b == 0 ) {
		*s = 0;
	}
	else {
		*s = 0;
	}
}

/**
 * Операция получить челое со знаком SGN троичного числа
 */
int8_t sgn(trs_t x) {
    int8_t i = sizeof(x.tb)*8;    
    while (1) {		
		if( ( (x.tb & 0x3) << (x.l<<2) ) > 0  ) {				
			break;
		}			
		if( i <= 0) {			
			break; 
		}			
		i -= 2;			
		x.tb = x.tb << 2;		
	};

	if ( (x.tb & (0x1 << (x.l<<2))) && (x.tb & (0x2 << (x.l<<2)))  ) {
		return 0; /* '+' */
	}
    else if ( x.tb & (0x1 << (x.l<<2)) ) {
		return -1; /* '-' */
	}
	else if ( x.tb & (0x2 << (x.l<<2)) ) {
		return 1; /* '+' */
	}
    	return 0; /* '0' */
}

/**
 * Операция получить знак SGN троичного числа
 */
trs_t sgn_trs(trs_t x) {	
	trs_t r;
    int8_t i;			
    uint8_t sg = 0;			

	for(i=0;i<x.l;i++) {		
		sg = x.tb>>((x.l-1-i)<<1) & 3;
		if( sg>0 ) {
			break; 
		} 
	}
		
	r.l = 1;
	
    if ( sg == 1 ) {
		set_trit(&r,1,-1);		
		return r; /* '-' */
	}
	else if ( sg == 2 ) {
		set_trit(&r,1,1);		
		return r; /* '+' */
	}
	else {	
		set_trit(&r,1,0);	
    	return r; /* '0' */
	}
}

/**
 * Операция OR trs
 */
trs_t or_trs(trs_t x, trs_t y) {

	trs_t r;

	int8_t i,j;
	int8_t a,b,s;

	if( x.l >= y.l) {
		j = x.l;
	} 
	else {
		j = y.l;
	}
			
	for(i = 0; i < j; i++) {
		r.tb <<= 2;
		a = trit2bit(x);
		b = trit2bit(y);
		and_t(&a, &b, &s);
		r.tb |= bit2tb(s); 		
	}

	return r;	
}

/**
 * Операции XOR trs
 */
trs_t xor_trs(trs_t x, trs_t y) {
	trs_t r;

	int8_t i,j;
	int8_t a,b,s;

	if( x.l >= y.l) {
		j = x.l;
	} 
	else {
		j = y.l;
	}
			
	for(i = 0; i < j; i++) {
		r.tb <<= 2;
		a = trit2bit(x);
		b = trit2bit(y);
		xor_t(&a, &b, &s);
		r.tb |= bit2tb(s); 		
	}

	return r;	
}

/**
 * Операция NOT trs 
 */
trs_t not_trs(trs_t x) {
	int8_t i;
	int8_t s;
	trs_t r;
	
	clear(&r);

	r.l = x.l;
	if( x.l <= 0) {
		return r;
	}

	for(i = 0; i < r.l; i++) {
		r.tb <<= 2;
		s = trit2bit(x);
		s = -s;				
		r.tb |= bit2tb(s); 		
	}

	return r;
}

/** 
 * Троичное NEG отрицания тритов
 */
trs_t neg_trs(trs_t t) {
	return not_trs(t);
} 

/** 
 * Троичный INC trs
 */
int8_t inc_trs(trs_t *t) {

	 int8_t res;
	 int8_t pos;
	 trs_t m;
	 trs_t n;
	 trs_t y;

	 n = *t;	
	 
	 clear(&m);
	 m.l=t->l;	 
	 pos = m.l;
	 set_trit(&m,pos,1);	 
	 y = add_trs(n,m);	 
	 *t = y; 

	 return 0;	 
}

/**
 * Операция DEC trs
 */
int8_t dec_trs(trs_t *t) {

	int8_t s;
	trs_t m;
	trs_t n;
	
	clear(&m);
	m.l=t->l;	
	set_trit(&m,m.l,1);	 
	m = sub_trs(*t,m);
	*t = m;

	return 0;
}

/**
 * Операция сдвига тритов
 * Параметр:
 * if(d > 0) then "Вправо" 
 * if(d == 0) then "Нет сдвига" 
 * if(d < 0) then "Влево" 
 * Возврат: Троичное число 
 */
trs_t shift_trs(trs_t t, int8_t d) {
	trs_t r;
	r=t;	
	if( d>0 ) {
		r.l = t.l;
		r=t;
		r.tb >>= d*2;		
	}
	else if( d<0 ) {
		r.l = t.l;
		r=t;
		r.tb <<= (-d)*2;
	}
	return r;
} 

/**
 * Троичное сложение тритов
 */
trs_t add_trs(trs_t x, trs_t y) {
	int8_t i,j;
	int8_t a,b,s,p0,p1;
	trs_t r;

	if( x.l >= y.l) {
		j = x.l;
	} 
	else {
		j = y.l;
	}
	
	r.l = j;
	r.tb = 0;

	p0 = 0;
	p1 = 0;
			
	for(i = 0; i < j; i++) {
		a = trit2bit(x);
		b = trit2bit(y);
		sum_t(&a, &b, &p0, &s, &p1);
		x.tb >>= 2;
		y.tb >>= 2;
		r.tb |= (trilong)(bit2tb(s) & 0x03) << (i*2);
		p0 = p1;
	}

	return r;
}

/** 
 * Троичное вычитание тритов
 */
trs_t sub_trs(trs_t x, trs_t y) {
	int8_t i,j;
	int8_t a,b,s,p0,p1;
	trs_t r;

	if( x.l >= y.l) {
		j = x.l;
	} 
	else {
		j = y.l;
	}
	
	r.l = j;
	r.tb = 0;

	p0 = 0;
	p1 = 0;
			
	for(i = 0; i < j; i++) {
		a = trit2bit(x);
		b = trit2bit(y);
		b = -b;
		sum_t(&a, &b, &p0, &s, &p1);
		x.tb >>= 2;
		y.tb >>= 2;
		r.tb |= (trilong)(bit2tb(s) & 0x03) << (i*2) ;
		p0 = p1;
	}
	
	return r;
}

/**
 * Троичное умножение тритов
 */
trs_t mul_trs(trs_t a, trs_t b) {
	trs_t r;
	//TODO реализовать
	b.l = 0;
	b.tb = 0;
	return r;
}

/** 
 * Троичное деление тритов
 */
trs_t div_trs(trs_t a, trs_t b) {
	trs_t r;
	//TODO реализовать
	r.l  = 0;
	r.tb = 0;
	return r;
}

/**
 * Проверить на переполнение 18-тритного числа
 */
int8_t over(trs_t x) {

	int8_t r;
	
	r = get_trit_int(x,1);
	r += get_trit_int(x,2);

	if( (r == -2) || (r == 2) ) {
		return 1; /* OVER Error  */
	}
	else {
		return 0;
	}
}


/** 
 * Получить бинарное поле трита в позиции 
 * троичного числа
 */
uint8_t get_trit( trs_t t, int8_t pos) {
	if( pos<=0 || pos>t.l || t.l==0 ) 
			return 0;
	t.tb = t.tb >> ( t.l - pos )*2;
	return trit2low(t);
}

/**
 * Получить целое со знаком трита в позиции 
 * троичного числа
 */
int8_t get_trit_int( trs_t t, int8_t pos) {
	if( pos<=0 || pos>t.l || t.l==0 ) { 
		return 0;
	}				
	t.tb >>= (t.l - pos )*2;
	return tb2int( trit2low(t) );
}

/** 
 * Установить трит как целое со знаком в позиции 
 * троичного числа
 */
void set_trit( trs_t *t, int8_t pos, int8_t v) {
	trilong bt;
	if( pos<=0 || pos>t->l || t->l==0 ) 
			return;
	bt = ~((trilong)3 << (t->l - pos )*2); 
	t->tb &= bt;
	t->tb |= (trilong)(bit2tb(v)) << (t->l - pos )*2;
}

/** 
 * Оперция присваивания троичных чисел в регистры
 */
void _copy_trit( trs_t *src, trs_t *dst) {
	//TODO по размеру числа выполнить правильное размещение позиций тритов
	view_short_reg(src,"src=");
	view_short_reg(dst,"dst=");

	if( src->l == dst->l ) {
		dst->tb = src->tb;
	}
	else if ( src->l < dst->l ) {
		dst->tb = 0;
		dst->tb	|= src->tb<<(dst->l - src->l);  
	}
	else { /* src->l > dst->l */
		dst->tb = 0;
		dst->tb	|= src->tb>>(src->l - dst->l);  
	} 		
}

/** 
 * Оперция присваивания троичных чисел в регистры
 */
void copy_trs( trs_t *src, trs_t *dst) {
	
	if( dst->l == src->l ) {
		*dst = *src;
		dst->l = src->l;
	} 
	else if( dst->l > src->l ) {
		trs_t t;
		t = slice_trs(*src,1,src->l);
		t.tb <<= 2*(dst->l - src->l);
		t.l = dst->l;
		*dst = t;
		
	}
	else { /* dst->l < src->l */
		dst->tb = src->tb << 2*(dst->l - src->l);
	}
	
}


/** 
 * Получить часть тритов из троичного числа
 */
trs_t slice_trs( trs_t t, int8_t p1, int8_t p2) {

	int8_t i,n;
	trs_t r;

	clear(&r);	
	r.l = 0;

	if( (t.l <= 0) || (t.l > SIZE_WORD_LONG )  ) {
		return r;  /* Error */
	}	
	if( (p1>p2) || ((p2-p1+1)> t.l) ) {
		return r; /* Error */
	}
	
	for(i=p1;i<=p2;i++) {
		r.tb <<= 2;
		r.tb |= get_trit(t,i);		
	} 	
	r.l=p2-p1+1;
	
	return r;
}

/**
 * Преобразование трита в номер зоны
 */
int8_t trit2zone(trs_t t) {
	switch ( t.tb & (trilong)3 ) {
		case 0:
		case 3: return 0;
			break;
		case 2: return 2;
			break;
		case 1: return 1;
	}
	return 0;
}

/**
 *  Дешифратор тритов в индекс строки зоны памяти
 */
int16_t addr_trit2addr_index(trs_t t) {

	int8_t i;
	int8_t n;

	t.tb &= (trilong)0x3FFFF;
	return t.tb >>= 2;
}

/**
 * Дешифратор тритов в индекс адреса памяти
 */
uint8_t zone_drum_to_index(trs_t z) {
   int8_t r = NUMBER_ZONE_DRUM >> 1;
   r += get_trit_int(z,1)*1;
   return r;   
}

/**
 * Дешифратор тритов в индекс адреса памяти
 */
uint8_t row_drum_to_index(trs_t z) {

   uint8_t r = 40;   
   r += get_trit_int(z,4)*1;
   r += get_trit_int(z,3)*3;
   r += get_trit_int(z,2)*9;
   r += get_trit_int(z,1)*27;
   
   //r = (r+1) * 2 / 3;
   //r -= 1;

   return r;
}

/**
 * Дешифратор трита в индекс адреса памяти FRAM
 */
uint8_t zone_fram_to_index_ver_1(trs_t z) {
   
   int8_t r = NUMBER_ZONE_FRAM >> 1;   
   
   r += get_trit_int(z,1)*1;
   
   if( r == 0) {
	   return 0; /* Зона 0,1  для чтения 18-трит */ 
   }
   else if(r == 1 ) {
		return 1;
   }   
   else {
		return 2;
   }   
}

/**
 * Дешифратор трита в индекс адреса физической памяти FRAM
 */
uint8_t zone_fram_to_index(trs_t z) {
   
   int8_t r;   
   
   r = get_trit_int(z,1)*1;
   
   if( r < 0) {
	   return 0;
   }
   else if(r == 0 ) {
		return 0;
   }   
   else {
		return 1;
   }   
}

/**
 * Дешифратор строки 9-тритов в зоне памяти FRAM
 */
uint8_t row_fram_to_index(trs_t z) {
  
   int8_t r = 40;      
   r += get_trit_int(z,4)*1;
   r += get_trit_int(z,3)*3;
   r += get_trit_int(z,2)*9;
   r += get_trit_int(z,1)*27;   
   
   return (uint8_t)r;
}

/**
 * Новый адрес кода машины
 */
trs_t next_address(trs_t c) {
	
	trs_t r;	
	r = c;
	
	if( get_trit_int(r,5) == 0 ) {
		/* 0 */		
		inc_trs(&r);	
	}
	else if(get_trit_int(r,5) >= 1 ) {
		/* + */
		inc_trs(&r);
		inc_trs(&r);
	}	
	else if( get_trit_int(r,5) <= -1 ) {
		/* - */
		inc_trs(&r);		
	}
	
	return r;
}

/**
 * Операция очистить память ферритовую
 */
void clean_fram(void) {
	
	int8_t zone;
	int8_t row;

	for(zone=0; zone < SIZE_PAGES_FRAM; zone++) {
		for(row=0; row < SIZE_PAGE_TRIT_FRAM; row++) {
			mem_fram[row][zone] = 0;			
		}
	}	
}

/**
 * Операция очистить память на магнитном барабане
 */
void clean_drum(void) {
	int8_t zone;
	int8_t row;	

	for(zone=0; zone < NUMBER_ZONE_DRUM; zone++) {
		for(row=0; row < SIZE_ZONE_TRIT_DRUM; row++) {
			mem_drum[zone][row] = 0;			
		}
	}
}

#if 0
/**
 * Функция "Читать троичное число из ферритовой памяти"
 */
trs_t ld_fram_ver1( trs_t ea ) {	
    
	uint8_t zind; 
	uint8_t rind; 
	int8_t eap5;
	trs_t zr;
	trs_t rr;
	trishort r;
	trs_t res;

	/* Зона памяти FRAM */
	zr.l = 1;
	zr = slice_trs(ea,1,1);
	zind = zone_fram_to_index(zr);

	/* Индекс строки в зоне памяти FRAM */
	rr.l = 4; 
	rr = slice_trs(ea,2,5);	
	
	res.tb ^= res.tb; /* 0 */	
	
	eap5 = get_trit_int(ea,5);
	if(  eap5 < 0 ) {
		/* Прочитать 18-тритное число */
		set_trit(&rr,4,0);
		rind = row_fram_to_index(rr);
		r = mem_fram[zind+1][rind+1]; /* прочитать 10...18 младшую часть 18-тритного числа */
		res.tb = r;		
		r = mem_fram[zind][rind];	  /* прочитать 1...9 старшую часть 18-тритного числа */			
		res.tb |= ((trilong)r)<<18 & 0xFFFFC0000ul;
		res.l  = 18;
	}
	if( eap5 == 0 ) {
		/* Прочитать старшую часть 18-тритного числа */
		rind = row_fram_to_index(rr);
		r = mem_fram[zind][rind];
		res.tb = r & 0x3FFFFul;
		res.l  = 9;		
	}
	else { /* eap5 > 0 */
		/* Прочитать младшую часть 18-тритного числа */
		rind = row_fram_to_index(rr);
		r = mem_fram[zind][rind];		/* read low part trits */
		res.tb = r & 0x3FFFF;
		res.l  = 9;		
	}	 

	return res;
}
#endif

/**
 * Функция "Читать троичное число из ферритовой памяти"
 */
trs_t ld_fram( trs_t ea ) {	
    
	uint8_t zind; 
	uint8_t rind; 
	int8_t eap5;
	trs_t zr;
	trs_t rr;
	trishort r;
	trs_t res;

	/* Зона памяти FRAM */
	zr = slice_trs(ea,5,5);
	zr.l = 1;
	zind = zone_fram_to_index(zr);

	/* Индекс строки в зоне памяти FRAM */
	rr = slice_trs(ea,1,4);	
	rr.l = 4; 
	rind = row_fram_to_index(rr);
	
	res.tb = 0;
	
	eap5 = get_trit_int(ea,5);
	if(  eap5 < 0 ) {
		/* Прочитать 18-тритное число */
		r = mem_fram[rind][zind+1]; /* прочитать 10...18 младшую часть 18-тритного числа */
		res.tb = r;		
		r = mem_fram[rind][zind];	  /* прочитать 1...9 старшую часть 18-тритного числа */			
		res.tb |= ((trilong)r)<<18 & (trilong)0xFFFFC0000;
		res.l  = 18;
	}
	else if( eap5 == 0 ) {
		/* Прочитать старшую часть 18-тритного числа */
		r = mem_fram[rind][zind];
		res.tb = r & (trishort)0x3FFFF;
		res.l  = 9;
	}
	else { /* eap5 > 0 */
		/* Прочитать младшую часть 18-тритного числа */
		r = mem_fram[rind][zind];		/* read low part trits */
		res.tb = r & (trishort)0x3FFFF;
		res.l  = 9;		
	}	 	
	return res;
}

#if 0
/**
 * Функция "Записи троичного числа в ферритовую память"
 */
void st_fram_ver_1( trs_t ea, trs_t v ) {	
	
	uint8_t zind;
	uint8_t rind;
	int8_t eap5;
	trs_t zr;
	trs_t rr;

	/* Зона памяти FRAM */
	zr.l = 1;
	zr = slice_trs(ea,1,1);
	zind = zone_fram_to_index(zr);
	view_short_reg(&zr," zr");

	/* Индекс строки в зоне памяти FRAM */
	rr.l = 4; 
	rr = slice_trs(ea,2,5);	

	eap5 = get_trit_int(ea,5);
	if( eap5 < 0 ) {
		/* Зпаисать 18-тритное число */
		set_trit(&rr,4,0);		
		rind = row_fram_to_index(rr);		
		view_short_reg(&rr," eap5 < 0 rr");
		printf(" z=%d, ind=%d\r\n",zind, rind);
		mem_fram[zind][rind+1] = (trishort)(v.tb & 0x3FFFFul);
		mem_fram[zind][rind]   = (trishort)(v.tb>>18 & 0x3FFFFul);
	}
	if( eap5 == 0 ) {		
		rind = row_fram_to_index(rr);
		view_short_reg(&rr," eap5 == 0 rr");
		printf(" z=%d, ind=%d\r\n",zind, rind);
		mem_fram[zind][rind]   = (trishort)(v.tb & 0x3FFFFul);
	}
	else { /* eap5 > 0 */
		rind = row_fram_to_index(rr);
		view_short_reg(&rr," eap5 > 0 rr");
		printf(" z=%d, ind=%d\r\n",zind, rind);
		mem_fram[zind][rind] = (trishort)(v.tb & 0x3FFFFul);
	}
}
#endif





/**
 * Функция "Записи троичного числа в ферритовую память"
 */
void st_fram( trs_t ea, trs_t v ) {	

	uint8_t zind;
	uint8_t rind;
	int8_t eap5;
	trs_t zr;
	trs_t rr;

	/* Зона физической памяти FRAM */
	zr = slice_trs(ea,5,5);
	zr.l = 1;
	zind = zone_fram_to_index(zr);

	/* Индекс строки в зоне физической памяти FRAM */	
	rr = slice_trs(ea,1,4);
	rr.l = 4;
	rind = row_fram_to_index(rr);

	eap5 = get_trit_int(ea,5);	
	if( eap5 < 0 ) {		
		/* Записать 18-тритное число */
		trilong r;				
		r =  shift_trs(v,9).tb;		
		mem_fram[rind][zind] = (trishort)r & (trishort)0x3FFFF;		
		mem_fram[rind][zind + 1] = (trishort)v.tb & (trishort)0x3FFFF;
	}
	else if( eap5 == 0 ) {		
		mem_fram[rind][zind] = (trishort)(v.tb & (trishort)0x3FFFF);
	}
	else { /* eap5 > 0 */		
		mem_fram[rind][zind] = (trishort)(v.tb & (trishort)0x3FFFF);
	}
}

/**
 * Операция чтения в память магнитного барабана
 */  
trs_t ld_drum( trs_t ea ) {
	//TODO
	uint8_t zind; 
	uint8_t rind; 
	trs_t zr;
	trs_t rr;
	trs_t res;

	zr = slice_trs(ea,1,1);	
	zr.l = 1; 
	zind = zone_drum_to_index(zr);

	rr = slice_trs(ea,2,5);	
	rr.l = 4; 

	res.tb = 0;
	
	rind = row_fram_to_index(rr);
	res.tb = mem_drum[zind][rind] & 0x3FFFF;
	res.l  = 9;

	return res;
}

/**
 * Операция записи в память магнитного барабана
 */
void st_drum( trs_t ea, trs_t v ) {
	//TODO
	uint8_t zind; 
	uint8_t rind; 
	trs_t zr;
	trs_t rr;

	zr.l = 1; 
	zr = slice_trs(ea,1,1);
	zind = zone_drum_to_index(zr);

	rr.l = 4; 
	rr = slice_trs(ea,2,5);	

	rind = row_drum_to_index(rr);
	mem_drum[zind][rind] = v.tb & 0x3FFFF;
}

/**
 * Операция записи в память магнитного барабана
 */
void set_drum( trs_t ea, trs_t v ) {
	//TODO
	uint8_t zind; 
	uint8_t rind; 
	trs_t zr;
	trs_t rr;

	zr.l = 1; 
	zr = slice_trs(ea,1,1);
	zind = zone_drum_to_index(zr);

	rr.l = 4; 
	rr = slice_trs(ea,2,5);	

	rind = row_drum_to_index(rr);
	mem_drum[zind][rind] = v.tb & 0x3FFFF;
}

/** ***********************************************
 *  Алфавит троичной симметричной системы счисления
 *  -----------------------------------------------
 *  Троичный код в девятеричной системы 
 *  -----------------------------------------------
 *  С символами: W, X, Y, Z, 0, 1, 2, 3, 4.
 */
uint8_t trit2lt( int8_t v )  {
	switch( v ) {
		case -4 : return 'W'; break;
		case -3 : return 'X'; break;
		case -2 : return 'Y'; break;
		case -1 : return 'Z'; break;
		case  0 : return '0'; break;
		case  1 : return '1'; break;
		case  2 : return '2'; break;
		case  3 : return '3'; break;
		case  4 : return '4'; break;
		default : return '0'; break;	
	}
}

/**
 * Вид '-','0','+' в десятичный код
 */
int8_t symtrs2numb( uint8_t c )  {
	switch( c ) {
		case '-' :
		 return -1 ; break;
		case '0' :
		 return 0; break;
		case '+' :
		 return 1; break;
		default  : return 0; break;	
	}
}

/**
 * Bp  Вид '-','0','+' 
 */
uint8_t numb2symtrs( int8_t v )  {
		
	if( v <= -1 ) {
		return '-';
	}
	else if( v == 0 ) {
		return '0';
	}
	else {
		return '+';		
	}
		
}

/**
 *  Вид '-','0','+' строка в десятичный код
 */
int8_t str_symtrs2numb( uint8_t * s )  {
	int8_t i;
	int8_t len;
	int8_t res = 0;
	
	len = strlen(s);
	
	for(i=0;i<len;i++) {
		res += symtrs2numb(s[i]) * pow3(i);
		printf("%c",s[i]); 
	}	
	
	return res;	
}

/**
 *  Cтроку вида '-','0','+'  в троичное число
 */
trs_t smtr(uint8_t * s) {

      int8_t i;
      int8_t trit;
      int8_t len,lenmax;
      trs_t t;

      len = strlen(s);
      lenmax = len;
      t.l = len;
      
      if(len > SIZE_WORD_LONG) {
       t.l = SIZE_WORD_LONG;
       lenmax = SIZE_WORD_LONG;
      }

      for(i=0;i<lenmax;i++){
         trit = symtrs2numb( *(s+(len-i-1)) );
         set_trit(&t,len-i,trit);
      }
      return t;
}

/**
 * Девятеричный вид в троичный код
 */
uint8_t * lt2symtrs( int8_t v )  {
	switch( v ) {
		case 'W' :
		case 'w' :
		 return "--" ; break;
		case 'X' :
		case 'x' :
		 return "-0"; break;
		case 'Y' :
		case 'y' :
		 return "-+"; break;
		case 'Z' :
		case 'z' :
		 return "0-"; break;		
		case '0' : return "00"; break;
		case '1' : return "0+"; break;
		case '2' : return "+-"; break;
		case '3' : return "+0"; break;
		case '4' : return "++"; break;
		default  : return "  "; break;	
	}
}

/**
 *  Троичный код в девятеричной системы 
 *
 *  С символами: W, X, Y, Z, 0, 1, 2, 3, 4. 
 */
uint8_t trit_to_lt( int8_t v )  {
	switch( v ) {
		case -4 : return 'W'; break;
		case -3 : return 'X'; break;
		case -2 : return 'Y'; break;
		case -1 : return 'Z'; break;
		case  0 : return '0'; break;
		case  1 : return '1'; break;
		case  2 : return '2'; break;
		case  3 : return '3'; break;
		case  4 : return '4'; break;
		default : return '0'; break;	
	}
}

/**
 * Троичный код в девятеричной системы 
 *
 */
int32_t trs_to_digit( trs_t *tr )  {

	int32_t l;    
    int8_t i;
    trs_t x;

    l = 0;
    x = *tr;	
    for( i=0; i<x.l ; i++ ) {		    						
			x = *tr;
			x.tb >>= (i*2);		
			l += trit2bit(x) * pow3(i);
    }
    
	return l;
}

/**
 * Троичный код в девятеричной системы 
 *
 *  С символами: Ж, Х, У, Ц, 0, 1, 2, 3, 4. 
 */
int32_t tb_to_digit( trishort tb )  {

	int32_t l;    
    int8_t i;

    l = 0;   
    for( i=0; i<9 ; i++ ) {		    						
			l += tb2int(tb >> (i*2)) * pow3(i);
    }
    
	return l;
}

/**
 * Девятеричный вид в троичный код
 */
void cmd_str_2_trs( uint8_t * syms, trs_t * r )  {
	
	uint8_t i;
	uint8_t symtrs_str[40];

	if( strlen(syms) != 5) {
		r->l  = 9;
		r->tb = 0;
		printf(" --- ERROR syms\r\n");
		return;
	}	

	sprintf(symtrs_str,"%2s%2s%2s%2s%2s",
			lt2symtrs(syms[0]),
			lt2symtrs(syms[1]),
			lt2symtrs(syms[2]),
			lt2symtrs(syms[3]),
			lt2symtrs(syms[4])			
		);	

	for(i=1;i<10;i++) {
		set_trit(r,i,symtrs2numb(symtrs_str[i]));		
	}		
}

/**
 * Печать троичного числа в строку
 */
void trit_to_str(trs_t t) {
	
	int8_t i,j,n;
	int8_t t0,t1;
	trs_t x;
	
	n = t.l + 1;

	for (i=0;i<n;i+=2) {
		x = t;	
		x.tb >>= (n-2-i)*2 ;
		t0 = trit2bit(x);
		x.tb >>= 2;
		t1 = trit2bit(x);

		printf("%c",trit_to_lt( t1*3 + t0 ));	
	}

	return;
}

/**
 * Печать троичного числа в строку
 */
void trit_to_symtrs(trs_t t) {
	
	int8_t i,j,n;
	int8_t t0;
	trs_t x;
	
	n = t.l;

	for (i=0;i<n;i++) {
		x = t;					
		x.tb >>= 2*(n-1-i);
		t0 = trit2bit(x);
		printf("%c", numb2symtrs(t0));
	}	
	return;
}

/**
 * Преобразовать триты с строку строки бумажной ленты
 *
 *  Пар:  v - триты
 *  Рез:  lp - строка линии ленты 
 */
void trit2linetape(trs_t v, uint8_t * lp) {	
}

/**
 * Преобразовать строку строки бумажной ленты в триты
 * 
 * Пар:  lp - строка линии ленты 
 * Рез:  return=0 - OK', return|=0 - Error
 * 		 v - триты	 
 */
uint8_t linetape2trit(uint8_t * lp, trs_t * v) {
	trs_t r;
	r.l  = 0;
	r.tb = 0;
	*v = r;
	return 0; /* OK' */
}

/**
 * Печать троичного регистра 
 *  
 */
void view_short_reg(trs_t *t, uint8_t *ch) {
	 int8_t i;
	 int8_t l;
	 trs_t tv;
	 
	 tv = *t;
	 printf("%s: ",(char *)ch);
	 //printf("%s=%i\t[",ch,tv.l);	 	 	 
	 if( tv.l <= 0 ) {
		 //printf("]\n");
		 return;
	 }
	 else if(tv.l > SIZE_WORD_LONG) {
		 l = SIZE_WORD_LONG;
	 }
	 else {
		 l = tv.l;
	 }
	 
	 printf("[");
	 i=0;	 
	 do {
		  tv = *t;
		  tv.tb >>= (l-1-i)*2 ; 
		  printf("%i",trit2bit(tv));          		    		  
		  //switch(trit2bit(tv)) {
		  //	  case -1: printf("-"); break;
		  //	  case  0: printf("0"); break;
		  //	  case  1: printf("+"); break;
		  //  }
		  i++;          
     } 
	 while( i < l );
	 printf("], ");
	 printf("(%li), ",(long int)trs_to_digit(t));	 
	 tv = *t;
	 trit_to_str(tv);	 
	 printf("\n");
}

/**
 * Печать на электрифицированную пишущую машинку
 * 'An electrified typewriter'
 */
void electrified_typewriter(trs_t t, uint8_t local) {
	
	int32_t code;
	
	/* Регист переключения Русский/Латинский */
	static uint8_t russian_latin_sw = 0;
	/* Регист переключения Буквенный/Цифровой */
	static uint8_t letter_number_sw = 0;
	/* Регист переключения цвета печатающей ленты */
	static uint8_t color_sw = 0;

	color_sw += 0;

	russian_latin_sw = local;
	code = trs_to_digit(&t);
	
	switch( code ) {
		case 6: /* t = 1-10 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","А");
							break;
						default:  /* number */ 
							printf("%s","6");
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","A");
							break;
						default:  /* number */ 
							printf("%s","6");
						break;	
					}
				break;
			}
		  	break;  

		case 7:  /* t = 1-11 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","В");
							break;
						default:  /* number */ 
							printf("%s","7");
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","B");							
							break;
						default:  /* number */ 
							printf("%s","7");
							break;	
					}
				break;
			}
		  	break;  

		case 8:  /* t = 10-1 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","С");
							break;
						default:  /* number */
							printf("%s","8");
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","C");							
							break;
						default:  /* number */ 
							printf("%s","8");
							break;	
					}
				break;
			}
		  	break;  

		case 9:  /* t = 100 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Д");							
							break;
						default:  /* number */ 
							printf("%s","9");							
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","D");														
							break;
						default:  /* number */ 
							printf("%s","9");							
							break;	
					}
				break;
			}
		  	break;  
		
		case 10:  /* t = 101 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Е");							
							break;
						default:  /* number */ 
							printf("%s"," ");							
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","E");														
							break;
						default:  /* number */ 
							printf("%s"," ");							
							break;	
					}
				break;
			}
		  	break;  

		case -12:  /* t = -1-10 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Б");							
							break;
						default:  /* number */ 
							printf("%s","-");							
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","F");														
							break;
						default:  /* number */ 
							printf("%s","-");														
							break;	
					}
				break;
			}
		  	break;  

		case -9:  /* t = -100 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Щ");
							break;
						default:  /* number */ 
							printf("%s","Ю");
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","G");							
							break;
						default:  /* number */ 
							printf("%s","/");							
							break;	
					}
				break;
			}
		  	break;  

		case -8:  /* t = -101 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Н");
							break;
						default:  /* number */ 
							printf("%s",",");
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","H");
							break;
						default:  /* number */ 
							printf("%s",".");
							break;	
					}
				break;
			}
		  	break;  

		case -6:  /* t = -110  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","I");
							break;
						default:  /* number */ 
							printf("%s","+");
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Л");							
							break;
						default:  /* number */ 
							printf("%s","+");
							break;	
					}
				break;
			}
		  	break;  

		case -5:  /* t = -111 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Ы");
							break;
						default:  /* number */ 
							printf("%s","Э");
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","J");							
							break;
						default:  /* number */ 
							printf("%s","V");
							break;	
					}
				break;
			}
		  	break;  

		case -4:  /* t = 0-1-1 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","К");							
							break;
						default:  /* number */ 
							printf("%s","Ж");							
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","K");														
							break;
						default:  /* number */ 
							printf("%s","W");														
							break;	
					}
				break;
			}
		  	break;  

		case -3:  /* t = 0-10  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Г");														
							break;
						default:  /* number */ 
							printf("%s","Х");														
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","L");														
							break;
						default:  /* number */ 
							printf("%s","X");														
							break;	
					}
				break;
			}
		  	break;  

		case -2:  /* t = 0-11  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","М");
							break;
						default:  /* number */ 
							printf("%s","У");
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","M");							
							break;
						default:  /* number */ 
							printf("%s","Y");							
							break;	
					}
				break;
			}
		  	break;  

		case -1:  /* t = 00-1  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","И");							
							break;
						default:  /* number */ 
							printf("%s","Ц");							
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","N");														
							break;
						default:  /* number */ 
							printf("%s","Z");														
							break;	
					}
				break;
			}
		  	break;  

		case 0:  /* t = 000  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Р");														
							break;
						default:  /* number */ 
							printf("%s","О");														
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","P");																					
							break;
						default:  /* number */ 
							printf("%s","O");																					
							break;	
					}
				break;
			}
		  	break;  

		case 1:  /* t = 001  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Й");																					
							break;
						default:  /* number */ 
							printf("%s","1");																					
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Q");																					
							break;
						default:  /* number */ 
							printf("%s","1");																					
							break;	
					}
				break;
			}
		  	break;  

		case 2:  /* t = 01-1  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Я");																					
							break;
						default:  /* number */ 
							printf("%s","2");																					
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","R");																					
							break;
						default:  /* number */ 
							printf("%s","2");																					
							break;	
					}
				break;
			}
		  	break;  

		case 3:  /* t = 010  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Ь");																					
							break;
						default:  /* number */ 
							printf("%s","3");																					
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","S");																					
							break;
						default:  /* number */ 
							printf("%s","3");																					
							break;	
					}
				break;
			}
		  	break;  

		case 4:  /* t = 011  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Т");																					
							break;
						default:  /* number */ 
							printf("%s","4");																					
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","T");																					
							break;
						default:  /* number */ 
							printf("%s","4");																					
							break;	
					}
				break;
			}
		  	break;  

		case 5:  /* t = 1-1-1 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","П");																					
							break;
						default:  /* number */ 
							printf("%s","5");																					
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","U");																					
							break;
						default:  /* number */ 
							printf("%s","5");																					
							break;	
					}
				break;
			}
		  	break;  

		case 13:  /* t = 111 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","Ш");																					
							break;
						default:  /* number */ 
							printf("%s","Ф");																					
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","(");																					
							break;
						default:  /* number */ 
							printf("%s",")");																					
							break;	
					}
				break;
			}
		  	break;  

		case -7:  /* t = -11-1 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","=");																					
							break;
						default:  /* number */ 
							printf("%s","х");																					
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","=");																					
							break;
						default:  /* number */ 
							printf("%s","x");																					
							break;	
					}
				break;
			}
		  	break;  

		case -11:  /* t = -1-11 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
						// переключить цвет черный
							break;
						default:  /* number */ 
						// переключить цвет красный
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","?");																					
							break;
						default:  /* number */ 
							printf("%s","?");																					
							break;	
					}
				break;
			}
		  	break;  

		case 12:  /* t = 110  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							letter_number_sw = 0;
							break;
						default:  /* number */ 
							letter_number_sw = 0;
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							letter_number_sw = 0;							
							break;
						default:  /* number */ 
							letter_number_sw = 0;
							break;	
					}
				break;
			}
		  	break;  

		case 11:  /* t = 11-1  */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							letter_number_sw = 1;
							break;
						default:  /* number */ 
							letter_number_sw = 1;
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							letter_number_sw = 1;
							break;
						default:  /* number */ 
							letter_number_sw = 1;
							break;	
					}
				break;
			}
		  	break;  

		case -10:  /* t = -10-1 */
			switch( russian_latin_sw ) {
				case 0: /* russian */
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","\r\n");
							break;
						default:  /* number */ 
							printf("%s","\r\n");
							break;	
					}
				break;
				default: /* latin */ 
					switch( letter_number_sw ) {
						case 0: /* letter */ 
							printf("%s","\r\n");
							break;
						default:  /* number */ 
							printf("%s","\r\n");
							break;	
					}
				break;
			}
		  	break;  
	}
}

/**
 * Печать регистров машины Сетунь-1958 
 */
void view_short_regs(void) {
	int8_t i;

	printf("[ Registers Setun-1958: ]\n");

	view_short_reg(&K," K");
	view_short_reg(&F," F");	
	view_short_reg(&C," C");
	view_short_reg(&W," W");
	view_short_reg(&S," S");	
	view_short_reg(&R," R");
	view_short_reg(&MB," MB");
}

/**
 * Печать памяти FRAM машины Сетунь-1958 
 */
void view_fram(trs_t ea) {
	int8_t j;
	trs_t tv;
	
	uint8_t zind; 
	uint8_t rind; 
	int8_t eap5;
	trs_t zr;
	trs_t rr;	
	trs_t res;
	trishort r;
	trishort t;
	
	/* Зона памяти FRAM */
	zr = slice_trs(ea,5,5);
	zr.l = 1;
	zind = zone_fram_to_index(zr);

	/* Индекс строки в зоне памяти FRAM */
	rr = slice_trs(ea,1,4);	
	rr.l = 4; 
	rind = row_fram_to_index(rr);
	
	r = mem_fram[rind][zind];			
	t = r;
	
	printf("ram[...] (%3d:%2d) = ", rind - SIZE_PAGE_TRIT_FRAM/2, zind );
	printf(" hex = %08p [",r);			
	
	j = 0;	 	 		
	do {		  
		printf("%i",tb2int(r >> (9-1-j)*2));          		    		  
		j++;          
	} while( j < 9 );
	
	printf("], ");			
	printf("(%li), ",(long int)tb_to_digit(t));	 	 	 		

	tv.l = 9;
	tv.tb = r & 0x3FFFF; 			
	trit_to_str(tv);
	printf("\n");		  		 		 
	
}

void dumpf( trs_t addr1, trs_t addr2) {
	
	trs_t ad1 = addr1;
	trs_t ad2 = addr2;
	int16_t a1 = trs_to_digit(&ad1);
	int16_t a2 = trs_to_digit(&ad2);
	//printf(" a1,a1 = %d", );

	if( (a2 >= a1) && ( a2 >= ZONE_M_FRAM_BEG && a2 <= ZONE_P_FRAM_END ) && ( a2 >= ZONE_M_FRAM_BEG && a2 <= ZONE_P_FRAM_END ) ) {		
		for( uint16_t i=0; i<(abs(a2-a1)); i++ ) {
			if( trit2bit(ad1)>=0 ) {
				view_fram(ad1);
			}
			inc_trs(&ad1);
			if( trit2bit(ad1)<0 ) {
				inc_trs(&ad1);
				i += 1;
			}
		}
	}
}

/**
 * Печать памяти FRAM машины Сетунь-1958 
 */
void dump_fram(void) {
	
	int8_t zone;
	int8_t row;
	int8_t j;
	trishort r;
	trishort t;
	trs_t tv;

	printf("\r\n[ Dump FRAM Setun-1958: ]\r\n");

	for(row=0; row < SIZE_PAGE_TRIT_FRAM; row++) {
		for(zone=0; zone < SIZE_PAGES_FRAM; zone++) {

			r = mem_fram[row][zone];			
			t = r;
			
			printf("ram[...] (%3d:%2d) = ", row - SIZE_PAGE_TRIT_FRAM/2, zone );
			printf(" hex = %08p [",r);			
			
			j = 0;	 	 		
			do {		  
		  		printf("%i",tb2int(r >> (9-1-j)*2));          		    		  
		  		j++;          
			} while( j < 9 );
	 		
			printf("], ");			
	 		printf("(%li), ",(long int)tb_to_digit(t));	 	 	 		

			tv.l = 9;
			tv.tb = r & 0x3FFFF; 			
			trit_to_str(tv);
			printf("\n");		  		 		 
		}
	}		
}

/**
 * Печать памяти DRUM машины Сетунь-1958 
 */
void dump_drum(void) {

	int8_t zone;
	int8_t row;
	int8_t j;
	trishort r;
	trishort t;

	for(zone=0; zone < NUMBER_ZONE_DRUM; zone++) {
		for(row=0; row < SIZE_ZONE_TRIT_DRUM; row++) {
			
			r = mem_drum[zone][row];			
			t = r;
			
			printf("drum[% 4i]  (%3d:%3d) = [", zone*SIZE_ZONE_TRIT_DRUM + row,zone-36,row-26);
	 		
			j = 0;	 
	 		do {		  
		  		printf("%i",tb2int(r >> (9-1-j)*2));          		    		  
		  		j++;          
			} while( j < 9 );
	 		
			printf("], ");
	 		printf("(%li),\t",(long int)tb_to_digit(t));	 	 
	 		printf("\n");		  		 		 
		}
	}
}

/** *******************************************
 *  Реалиазция виртуальной машины "Сетунь-1958"
 *  -------------------------------------------
 */

/** 
 * Аппаратный сброс.
 * Очистить память и регистры
 * виртуальной машины "Сетунь-1958"
 */
void reset_setun_1958(void) {
	// 
	clean_fram();	/* Очистить  FRAM */
	clean_drum();	/* Очистить  DRUM */
	//
	clear(&K);		/* K(1:9) */
	K.l = 9;
	clear(&F);		/* F(1:5) */
	F.l = 5;
	clear(&C);		/* K(1:5) */
	C.l = 5;
	clear(&W);		/* W(1:1) */
	W.l = 1;  					
	//	
	clear(&S);		/* S(1:18) */
	S.l = 18;  
	clear(&R);		/* R(1:18) */
	R.l = 18;  				
	clear(&MB);		/* MB(1:4) */
	MB.l = 4;  				
	//
	clear(&MR);		/* Временный регистр данных MR(1:9) */
	MR.l = 9;
}

/** 
 * Вернуть модифицированное K(1:9) для выполнения операции "Сетунь-1958"
 */
trs_t control_trs( trs_t a ) {
		int8_t k9;
		trs_t k1_5;
		trs_t r;
		trs_t cn;

		k1_5 = slice_trs(a,1,5);
		/* Признак модификации адремной части K(9) */
		k9 = trit2bit(a); 
		
		/* Модицикация адресной части K(1:5) */
		if( k9 >= 1 ) { 	/* A(1:5) = A(1:5) + F(1:5) */ 			
			cn = add_trs(k1_5,F);
			cn.tb <<= 4*2;
			r.tb = a.tb & 0xFF; 		/* Очистить неиспользованные триты */
			r.tb |= cn.tb & 0x3FF00 ;
		}
		else if( k9 <= -1 ) {	/* A(1:5) = A(1:5) - F(1:5) */
			cn = sub_trs(k1_5,F);
			cn.tb <<= 4*2;
			r.tb = a.tb & 0xFF; 		/* Очистить неиспользованные триты */
			r.tb |= cn.tb & 0x3FF00 ;
		} 
		else {					/* r = K(1:9) */
			r = a;
		}

		r.l = 9;

		return r;
}

/***************************************************************************************
                   Таблица операций машина "Сетунь-1958"
----------------------------------------------------------------------------------------
Код(троичный)     Код        Название операции           W       Содержание команды 
+00        30         9        Посылка в S               w(S)    (A*)=>(S)
+0+        33        10        Сложение в S              w(S)    (S)+(A*)=>(S)
+0-        3х         8        Вычитание в S             w(S)    (S)-(A*)=>(S)
++0        40        12        Умножение 0               w(S)    (S)=>(R); (A*)(R)=>(S)
+++        43        13        Умножение +               w(S)    (S)+(A*)(R)=>(S)
++-        4х        11        Умножение -               w(S)    (A*)+(S)(R)=>(S)
+-0        20         6        Поразрядное умножение     w(S)    (A*)[x](S)=>(S)
+-+        23         7        Посылка в R               w(R)    (A*)=>(R)
+--        2х         5        Останов    Стоп;          w(R)    STOP; (A*)=>(C)
0+0        10         3        Условный переход 0         -      A*=>(C) при w=0
0++        13         4        Условный переход +         -      A*=>(C) при w=+
0+-        1х         2        Условный переход -         -      A*=>(C) при w=-
000        00         0        Безусловный переход        -      A*=>(C)
00+        03         1        Запись из C                -      (C)=>(A*)
00-        0х        -1        Запись из F               w(F)    (F)=>(A*)
0-0        ц0        -3        Посылка в F               w(F)    (A*)=>(F)
0--        цх        -4        Сложение в F              w(F)    (F)+(A*)=>(F)
0-+        ц3        -2        Сложение в F c (C)        w(F)    (C)+(A*)=>(F)
-+0        у0        -6        Сдвиг (S) на              w(S)    (A*)=>(S)
-++        у3        -5        Запись из S               w(S)    (S)=>(A*)
-+-        ух        -7        Нормализация              w(S)    Норм.(S)=>(A*); (N)=>(S)
-00        х0        -9        Вывод-ввод    Ввод в Фа*.  -      Вывод из Фа*
-0+        х3        -8        Запись на МБ               -      (Фа*)=>(Мд*)
-0-        хх        -10        Считывание с МБ           -      (Мд*)=>(Фа*)
--0        ж0        -12        Не задействована                 Аварийный стоп
--+        ж3        -11        Не задействована                 Аварийный стоп
---        жх        -13        Не задействована                 Аварийный стоп
------------------------------------------------------------------------------------------
*/
/**
 * Выполнить операцию K(1:9) машины "Сетунь-1958"
 */
int8_t execute_trs( trs_t addr, trs_t oper ) {
//TODO для С(5) = -1 выполнить 2-раза страшей половине A(9:18) и сделать inc C

		trs_t  k1_5; 		/* K(1:5)	*/
		trs_t  k6_8; 		/* K(6:8)	*/
		int8_t codeoper;	/* Код операции */

		/* Адресная часть */	
		k1_5 = slice_trs(addr,1,5);
		k1_5.l = 5;

		/* Код операции */	
		k6_8 = oper;
		k6_8.l = 3;

		codeoper = get_trit_int(k6_8,1)*9 +
		           get_trit_int(k6_8,2)*3 +
				   get_trit_int(k6_8,3);		
		
		/* ---------------------------------------
		*  Выполнить операцию машины "Сетунь-1958"
		*  ---------------------------------------
		*/

		/*
		* Описание реализации команд машины «Сетунь»
		*
		* 5-разрядный регистр управления С, в котором содержится адрес
		* выполняемой команды, после выполнения каждой команды в регистре С
		* формируется адрес следующей команды причём за командой являющейся первой
		* коротким кодом какой-либо ячейки, следует­ команда, являющаяся вторым 
		* коротким кодом этой ячейки, а вслед за ней — ко­манда, являющаяся первым
		* коротким кодом следующей ячейки, и т. д.;
		* этот порядок может быть нарушен при выполнении команд
		* перехода.
		*
		* При выполнении команд, использую­щих регистры F и С, операции производятся
		* над 5-разрядными кодами, которые можно рассматривать как целые числа. 
		* При выборке из памяти 5-разрядный код рассматривается как старшие пять разрядов 
		* соответст­вующего короткого или длинного кода, при запоминании в ячейке 
		* па­мяти 5-разрядный код записывается в старшие пять разрядов и допол­няется
		* до соответствующего короткого или длинного кода записью нулей в остальные разряды.
		* 
		* При выполнении команд, использующих регистры S и R, выбираемые из памяти короткие коды
		* рассматриваются как длин­ные с добавлением нулей в девять младших разрядов, а в оперативную
        * память в качестве короткого кода записывается содержимое девяти старших разрядов регистра S 
		* (запись в оперативную память непосред­ственно из регистра R невозможна).
		*		
		* После выполнения каждой команды вырабатывается значение неко­торого признака W = W(X) {-1,0,1}, 
		* где X — обозначение какого-либо регистра, или сохраняется предыду­щее значение этого признака.		  
        * 
		* Команды условного перехода выполняются по-разному в зависимости от значения этого признака W.
		* 
		* При выполнении операций сложения, вычитания и умножения, использующих регистр S,
		* может произойти останов машины по переполнению, если результат выполнения
		* этой опе­рации, посылаемый в регистр S, окажется по модулю больше 4,5 .
		* 
		* Операция сдвига производит сдвиг содержимого регистра S на |N|-разрядов, где N рассматривается
		* как 5-разрядный код, хранящийся в ячейке A*, т.е. N = (А*). Сдвиг производится влево при N > 0 и
		* вправо при N < 0. При N = 0 содержимое регистра S не изменяется.
		* 
		* Операция нормализации производит сдвиг (S) при (S) != 0 в таком направлении и на такое число
		* разрядов |N|, чтобы результат, посылаемый в ячейку A*, был но модулю больше 1/2 , но меньше 3/2,
		* т.е. чтобы в двух старших разрядах результата была записана комбинация троичных цифр 01 или 0-1.
		* При этом в регистр S посылается число N (5-разрядный код), знак которого определяется 
		* направлением сдвига, а именно: N > 0 при сдвиге вправо и N < 0 при сдвиге влево. При (S) = 0 или при
	    * 1/2 <|(S)| < 3/2 в ячейку А* посылается (S), а в регистр S посылается N = 0.
		*
		* Остальные операции, содержащиеся в табл. 1, ясны без дополни­тельных пояснений.
		*
		*/
		
		printf("A*=["); trit_to_symtrs(k1_5);
		printf("]");
		printf(", (% 4li), ",(long int)trs_to_digit(&k1_5));
		
		switch( codeoper ) {
			case (+1*9 +0*3 +0):  { // +00 : Посылка в S	(A*)=>(S)
				printf("   k6..8[+00] : (A*)=>(S)\n");
				MR = ld_fram(k1_5);
				copy_trs(&MR,&S);
				W = sgn_trs(S);
				C = next_address(C);								
			} break;
			case (+1*9 +0*3 +1):  { // +0+ : Сложение в S	(S)+(A*)=>(S)
				printf("   k6..8[+0+] : (S)+(A*)=>(S)\n");
				MR = ld_fram(k1_5);				
				S = add_trs(S,MR);
				W = sgn_trs(S);
				if( over(S) > 0 ) {					
					goto error_over;
				}
				C = next_address(C);
			} break;
			case (+1*9 +0*3 -1):  { // +0- : Вычитание в S	(S)-(A*)=>(S)
				printf("   k6..8[+0-] : (S)-(A*)=>(S)\n");
				MR = ld_fram(k1_5);
				
				view_short_reg(&S,"S - =");
				view_short_reg(&MR,"MR=");

				S = sub_trs(S,MR);				
				W = sgn_trs(S);
				if( over(S) > 0 ) {					
					goto error_over;
				}
				C = next_address(C);
			} break;
			case (+1*9 +1*3 +0):  { // ++0 : Умножение 0	(S)=>(R); (A*)(R)=>(S)
				printf("   k6..8[++0] : (S)=>(R); (A*)(R)=>(S)\n");
				copy_trs(&S,&R);				
				MR = ld_fram(k1_5);				
				//S = mul_trs(MR,R); //TODO
				W = sgn_trs(S);
				if( over(S) > 0 ) {
					goto error_over;
				} 
				C = next_address(C);
			} break;
			case (+1*9 +1*3 +1):  { // +++ : Умножение +	(S)+(A*)(R)=>(S)
				printf("   k6..8[+++] : (S)+(A*)(R)=>(S)\n");
				MR = ld_fram(k1_5);				
				//R = mul_trs(MR,R); //TODO реализовать
				S = add_trs(S,R);
				W = sgn_trs(S);
				if( over(S) > 0 ) {
					goto error_over;
				} 
				C = next_address(C);
			} break;
			case (+1*9 -1*3 +0):  { // +-0 : Поразрядное умножение	(A*)[x](S)=>(S)
				printf("   k6..8[+-0] : (A*)[x](S)=>(S)\n");
				MR = ld_fram(k1_5);
				S = xor_trs(MR,S);
				W = sgn_trs(S);
				C = next_address(C);
			} break;
			case (+1*9 -1*3 +1):  { // +-+ : Посылка в R	(A*)=>(R)
				printf("   k6..8[+-+] : (A*)=>(R)\n");
				MR = ld_fram(k1_5);
				copy_trs(&MR,&R);
				W = sgn_trs(S);
				C = next_address(C);
			} break;
			case (+1*9 -1*3 -1):  { // +-- : Останов	Стоп; (A*)=>(R)
				printf("   k6..8[+--] : (A*)=>(R)\n");
				MR = ld_fram(k1_5);
				copy_trs(&MR,&R); 
				C = next_address(C);
			} break;
			case (+0*9 +1*3 +0):  { // 0+0 : Условный переход -	A*=>(C) при w=0
				printf("   k6..8[0+-] : A*=>(C) при w=0\n");
				uint8_t w;
				w = sgn(W);
				if( w==0 ) {
					copy_trs(&k1_5,&C); 
				}
				else {
					C = next_address(C);
				} 
			} break;
			case (+0*9 +1*3 +1):  { // 0+1 : Условный переход -	A*=>(C) при w=0
				printf("   k6..8[0+-] : A*=>(C) при w=+1\n");
				uint8_t w;
				w = sgn(W);
				if( w==1 ) {
					copy_trs(&k1_5,&C); 
				}
				else {
					C = next_address(C);
				} 
			} break;
			case (+0*9 +1*3 -1):  { // 0+- : Условный переход -	A*=>(C) при w=-
				printf("   k6..8[0+-] : A*=>(C) при w=-1\n");
				uint8_t w;
				w = sgn(W);
				if( w<0 ) {
					copy_trs(&k1_5,&C); 
				}
				else {
					C = next_address(C);
				} 
			} break;
			case (+0*9 +0*3 +0): { //  000 : Безусловный переход	A*=>(C)
				printf("   k6..8[000] : A*=>(C)\n");
				copy_trs(&k1_5,&C); 
			} break;
			case (+0*9 +0*3 +1):  { // 00+ : Запись из C	(C)=>(A*)
				printf("   k6..8[00+] : (C)=>(A*)\n");
				st_fram(k1_5,C); 
				C = next_address(C);
			} break;
			case (+0*9 +0*3 -1):  { // 00- : Запись из F	(F)=>(A*)
				printf("   k6..8[00-] : (F)=>(A*)\n");
				st_fram(k1_5,F);
				W = sgn_trs(F); 
				C = next_address(C);
			} break;
			case (+0*9 -1*3 +0):  { // 0-0 : Посылка в F	(A*)=>(F)
				printf("   k6..8[0-0] : (A*)=>(F)\n");
				MR = ld_fram(k1_5);
				copy_trs(&MR,&F);
				W = sgn_trs(F);
				C = next_address(C);
			} break;
			case (+0*9 -1*3 +1):  { // 0-+ : Сложение в F c (C)	(C)+(A*)=>F
				printf("   k6..8[0-+] : (C)+(A*)=>F\n");
				MR = ld_fram(k1_5);
				F = add_trs(C,MR);
				W = sgn_trs(F);
				C = next_address(C);
			} break;
			case (+0*9 -1*3 -1):  { // 0-- : Сложение в F	(F)+(A*)=>(F)
				printf("   k6..8[0--] : (F)+(A*)=>(F)\n");
				MR = ld_fram(k1_5);
				F = add_trs(F,MR);
				W = sgn_trs(F);
				C = next_address(C);
			} break;
			case (-1*9 +1*3 +0):  { // -+0 : Сдвиг	Сдвиг (S) на (A*)=>(S)
				printf("   k6..8[-+0] : (A*)=>(S)\n");
				/*
				* Операция сдвига производит сдвиг содержимого регистра S на \N\
				* разрядов, где N рассматривается как 5-разрядный код, хранящийся в
				* ячейке Л*, т. е. N = (А*). Сдвиг производится влево при N > 0 и вправо
				* при N < 0. При N = 0 содержимое регистра 5 не изменяется.
				*/
				MR = ld_fram(k1_5);
				//TODO add  S = shift_trs(S,trit2dec(MR));				
				W = sgn_trs(S);
				C = next_address(C);
			} break;
			case (-1*9 +1*3 +1):  { // -++ : Запись из S	(S)=>(A*)
				printf("   k6..8[-++] : (S)=>(A*)\n");
				st_fram(k1_5,S);
				W = sgn_trs(S);
				C = next_address(C);
			} break;
			case (-1*9 +1*3 -1):  { // -+- : Нормализация	Норм.(S)=>(A*); (N)=>(S)
				printf("   k6..8[-+-] : Норм.(S)=>(A*); (N)=>(S)\n");
				/*
				* Операция нормализации производит сдвиг (S) при (5) =£= 0 в таком
				* направлении и на такое число разрядов |iV|, чтобы результат, посылаемый
				* в ячейку ,4*, был но модулю больше V , но меньше / , т. е. чтобы в д в у х
				* старших разрядах результата была записана комбинация троичных цифр
				* 01 или 0 1 . При этом в регистр S посылается число N (5-разрядный код),
				* знак которого определяется направлением сдвига, а именно: N > 0 при
				* сдвиге вправо и N < 0 при сдвиге влево. П р и (S) = 0 или при
				* / <|(*S)| < / в ячейку А* посылается (5), а в регистр S посылается N = 0.				
				*/
				//TODO описание операции
				//
				W = sgn_trs(S); 
				C = next_address(C);				
			} break;
			case (-1*9 +0*3 +0):  { // -00 : Не задействована	Стоп
				printf("   k6..8[-00] : STOP\n");
				return STOP_ERROR;
			} break;
			case (-1*9 +0*3 +1):  { // -0+ : Запись на МБ	(Фа*)=>(Мд*)
				printf("   k6..8[-0+] : (Фа*)=>(Мд*)\n");
				C = next_address(C);
			} break;
			case (-1*9 +0*3 -1):  { // -0- : Считывание с МБ	(Мд*)=>(Фа*)
				printf("   k6..8[-0-] : (Мд*)=>(Фа*)\n");
				C = next_address(C);
			} break;
			case (-1*9 -1*3 +0):  { // --0 : Не задействована	Стоп
				printf("   k6..8[--0] : STOP BREAK\n");
				return STOP_ERROR;
			} break;
			case (-1*9 -1*3 +1):  { // --+ : Не задействована	Стоп
				printf("   k6..8[--+] : STOP BREAK\n");
				return STOP_ERROR;
			} break;
			case (-1*9 -1*3 -1):  { // --- : Не задействована	Стоп
				printf("   k6..8[---] : STOP BREAK\n");
				return STOP_ERROR;
			} break;
			default: {				// Не допустимая команда машины
				printf("   k6..8 =[]   : STOP! NO OPERATION\n");
				return STOP_ERROR; 
			} 
			break;
		}	
	return OK;				
	
	error_over:
	return STOP_ERROR;				

}


/** *********************************************
 *  Тестирование виртуальной машины "Сетунь-1958"
 *  типов данных, функции
 *  ---------------------------------------------
 */
void Triniti_tests( void ) {

	printf("\n --- START Triniti tests VM SETUN-1958 --- \n");

	printf("pR=%08p\n",&R);

	//t1 Point address
	printf("\nt1 --- Point type addr\n");
	addr pM;
	pM = 0xffffffff;
	printf("pM = 0xffffffff [%ji]\n",pM);

	//t2 XOR-троичная двоичными операциями
	printf("\nt2 --- XOR\n");
	int16_t a,b,c;
	printf("a = 0x10\n");
	printf("b = 0x10\n");
	printf("c = a | b\n");
	printf("c = c ^ 0x11\n");	
	a = (int16_t)0x10;
	b = (int16_t)0x10;
	c = a | b;
	c = c ^ (int16_t)0x11;
	printf("a xor b = c  0x%x\n",(int8_t)c );

	//t3 POW3
	printf("\nt3 --- POW3\n");
	printf("pow3(3)=%li\n", (int32_t)pow3(3));

	//t4 trit2bit
	printf("\nt4 --- trit2bit()\n");
	trs_t k;
	k.tb = 0;
	printf("trit2bit(0)=%i\n", trit2bit(k));
	k.tb = 1;
	printf("trit2bit(1)=%i\n", trit2bit(k));
	k.tb = 2;
	printf("trit2bit(2)=%i\n", trit2bit(k));
	k.tb = 3;
	printf("trit2bit(3)=%i\n", trit2bit(k));

	//t5
	printf("\nt5 --- bit2trit()\n");
	k = bit2trit(0);
	printf("bit2trit(0)=%lu\n", k.tb);
	k = bit2trit(1);
	printf("bit2trit(1)=%lu\n", k.tb);
	k = bit2trit(-1);
	printf("bit2trit(-1)=%lu\n", k.tb);

	//t6
	printf("\nt6 --- printf long int\n");
	long int ll = 	0x00000001lu;
	printf("long int ll = 	0x00000001lu\n");
	printf("ll=%lu\n",ll);
	ll <<= 2;
	printf("ll=%lu\n",ll);
	ll <<= 1;
	printf("ll=%lu\n",ll);

	//t7
	printf("\nt7 --- sng()\n");
	k.tb = 0;
	printf("k.tb = 0\n");
	printf("sgn(k)=%i\n",sgn(k));
	k.tb = 1;
	printf("k.tb = 1\n");
	printf("sgn(k)=%i\n",sgn(k));
	k.tb = 2;
	printf("k.tb = 2\n");
	printf("sgn(k)=%i\n",sgn(k));
	k.tb = 3;
	printf("k.tb = 3\n");
	printf("sgn(k)=%i\n",sgn(k));

	//t8	
	printf("\nt8 --- st_fram()\n");
	C.l = 5;
	C.tb = 0;
	//	
	k.l = 9;
	k.tb = 4;
	//
	view_short_reg(&C,"C =");
	view_short_reg(&k,"k =");
	printf("st_fram(C,k)\n");
	st_fram(C,k);

	//t9	
	printf("\nt9 --- control_trs()\n");
	C.l = 5;
	C.tb = 0;

	C.l = 5;
	C.tb = 0;

	//t control
	trs_t anw;
	anw.l = 9;
	anw.tb = 0;
	//
	F.l = 5;
	F.tb = 2;
	//	
	k.l = 9;
	k.tb = 2<<8;
	k.tb |= 2;
	view_short_reg(&k,"k");
	anw = control_trs( k );
	view_short_reg(&C,"C");
	view_short_reg(&F,"F");
	view_short_reg(&anw,"anw");

	//t10
	printf("\nt10 --- get_trit()\n");
	uint8_t fl;
	view_short_reg(&k,"k");
	fl = get_trit(k,9);
	printf(" fl=%i",fl);
	fl = get_trit(k,8);
	printf(" fl=%i",fl);
	fl = get_trit(k,7);
	printf(" fl=%i",fl);
	fl = get_trit(k,6);
	printf(" fl=%i",fl);
	fl = get_trit(k,5);
	printf(" fl=%i",fl);
	fl = get_trit(k,4);
	printf(" fl=%i",fl);
	fl = get_trit(k,3);
	printf(" fl=%i",fl);
	fl = get_trit(k,2);
	printf(" fl=%i",fl);
	fl = get_trit(k,1);
	printf(" fl=%i",fl);
	printf("\n");

	//t11
	printf("\nt11 --- slice_trs()\n");
	trs_t q;
	view_short_reg(&k,"k");
	q = slice_trs(k,1,9);
	view_short_reg(&q,"q");
	q = slice_trs(k,6,8);
	view_short_reg(&q,"q");
	q = slice_trs(k,5,5);
	view_short_reg(&q,"q");
	q = slice_trs(k,9,9);
	view_short_reg(&q,"?q");
	q = slice_trs(k,4,4);
	view_short_reg(&q,"??q");

	//t12
	printf("\nt12 --- set_trit()\n");
	trs_t H;
	H.l=9;
	H.tb = 0;
	set_trit( &H, 1, 1);
	set_trit( &H, 2, -1);
	set_trit( &H, 3, 0);
	set_trit( &H, 9, -1);
	view_short_reg(&H,"H");

	//t13 fram
	printf("\nt13 --- st_fram() ld_fram\n");
	
	int status = 0;
	int cdoper = 0;
	int cnt = 0;

	status += 0;
	cdoper += 0;
	cnt += 0;

	trs_t ea;
	clear(&ea);
	ea.l = 5;

	trs_t v;
	clear(&v);
	v.l = 9;
	set_trit(&v,1,-1);
	set_trit(&v,5,1);
	set_trit(&v,9,0);
	//
	st_fram( ea,v );
	v.l = 9;
	set_trit(&v,1,0);
	set_trit(&v,5,-1);
	set_trit(&v,9,10);
	inc_trs(&ea);
	st_fram( ea,v );

	//t14 fram
	printf("\nt14 --- st_fram() ld_fram\n");
	
	clear(&ea);
	clear(&R);

	ea.l = 5;
	set_trit(&ea,5,1);
	R.l = 18;
	set_trit(&R,1,1);
	set_trit(&R,18,-1);

	view_short_reg(&ea,"st ea");
	view_short_reg(&R,"st R");

	st_fram(ea,R);
	R = ld_fram(ea);
	view_short_reg(&ea," ld ea");
	view_short_reg(&R," ld R");

	//t15 fram
	printf("\nt15 --- st_fram() ld_fram\n");

	set_trit(&R,1,1);
	set_trit(&R,10,-1);
	inc_trs(&ea);
	view_short_reg(&ea,"ea=");
	view_short_reg(&R,"R=");	
	printf("st_fram(ea,R)\n");
	st_fram(ea,R);
	R = ld_fram(ea);
	printf("R = ld_fram(ea)\n");
	view_short_reg(&R,"R");

	//t16 fram
	printf("\nt16 --- smtr() ld_fram\n");

	//
	trs_t X;
	trs_t in;

	printf("X = smtr(\"0000-----\")\n");
    
	X = smtr("0000-----");

	in = shift_trs(X, 1);
	view_short_reg(&in,"in");
	in = shift_trs(X, 2);
	view_short_reg(&in,"in");
	in = shift_trs(X, 3);
	view_short_reg(&in,"in");
	in = shift_trs(X, 4);
	view_short_reg(&in,"in");
	in = shift_trs(X, 5);
	view_short_reg(&in,"in");

	set_trit(&X,9,-1);
	in = shift_trs(X, -1);
	view_short_reg(&in,"in");
	in = shift_trs(X, -2);
	view_short_reg(&in,"in");
	in = shift_trs(X, -3);
	view_short_reg(&in,"in");
	in = shift_trs(X, -4);
	view_short_reg(&in,"in");
	in = shift_trs(X, -5);
	view_short_reg(&in,"in");
	in = shift_trs(X, -6);
	view_short_reg(&in,"in");
	in = shift_trs(X, -7);
	view_short_reg(&in,"in");

	printf("X = smtr(\"00000000-\")\n");

    X = smtr("00000000-");

	clear(&in);
	in.l=9;
	set_trit(&in,9,-1);
	view_short_reg(&in," in");
	view_short_reg(&X," X");
	in = add_trs(X, in);
	view_short_reg(&in,"add in");

	set_trit(&in,9,-1);

	view_short_reg(&in," in");
	view_short_reg(&X," X");
	in = add_trs(X, in);
	view_short_reg(&in,"add in");
	in = add_trs(in, in);
	view_short_reg(&in,"add in");


	//
	clear(&in);
	clear(&X);
	in.l=5;
	X.l=9;
    
	in = smtr("-----");
    X = smtr("0000-----");

	view_short_reg(&in,"in=");
	view_short_reg(&X,"X=");

	clean_fram();

	in.l = 18;
	inc_trs(&in);
	view_short_reg(&in," m in");
	inc_trs(&in);
	view_short_reg(&in," m in");

	int l;
	clean_fram();
	clear(&X);
	clear(&in);
	X.l  = 18;
	set_trit(&X,18,-1);
	set_trit(&X,1,-1);
	in = smtr("00011");

	st_fram(in,X);
	set_trit(&in,1,0);
	set_trit(&in,2,0);
	set_trit(&in,3,0);
	set_trit(&in,4,1);
	set_trit(&in,5,0);
	st_fram(in,X);

	view_short_reg(&in," in");
	view_short_reg(&X,"  X");


	//t17 fram
	printf("\nt17 --- Write/Read fram\n");

	clear(&X);
	clear(&in);
	X.l  = 18;
	set_trit(&X,1,-1);
	set_trit(&X,18,-1);
	in.l = 5;
	set_trit(&in,1,-1);
	set_trit(&in,2,-1);
	set_trit(&in,3,-1);
	set_trit(&in,4,-1);
	set_trit(&in,5,-1);

	printf("\nst_fram(in,X)\r\n");

	for(l=-121;l<=+121;l++) { //TRIT5_MIN TRIT5_MAX

		if( get_trit_int(in,5) == -1 ) {
			view_short_reg(&in," addr");
			st_fram(in,X);
		}
		inc_trs(&in);
		inc_trs(&X);
	}
	
	dump_fram();

	//
	view_short_reg(&k,"in k");
	dec_trs(&k);
	dec_trs(&k);
	view_short_reg(&k,"out k");

	printf("\n --- VM Setun-1958 Executes --- \n");

	reset_setun_1958();

	clear(&k);
	k.l = 9;
	set_trit(&k,1,-1);
	set_trit(&k,2,-1);
	set_trit(&k,3,-1);
	set_trit(&k,4,-1);
	set_trit(&k,5,-1);
	set_trit(&k,6,-1);
	set_trit(&k,7,-1);
	set_trit(&k,8,-1);
	set_trit(&k,9,-1);

	//view_short_reg(&k," k");
	//C = control_trs(k);
	//view_short_reg(&C," C");

	//for(l=TRIT9_MIN;l<=TRIT9_MAX;l++) {
	//	inc_trs(&k);
	//	status = execute_trs( control_trs(k) );
	//}

	view_short_regs();

	//t
	clean_drum();
	clean_fram();
	//printf(" --- DUMP MEM FRAM --- \n");
	//dump_fram();
	//dump_drum();

	printf("\n --- TEST electrified_typewriter() --- \n");

	trs_t cp;
	clear(&cp);
	cp.l = 3;
	set_trit(&cp,1,-1);
	set_trit(&cp,2,-1);
	set_trit(&cp,3,-1);

	uint8_t cc;

	set_trit(&cp,1,1);
	set_trit(&cp,2,1);
	set_trit(&cp,3,0);
	electrified_typewriter(cp,0);

	set_trit(&cp,1,-1);
	set_trit(&cp,2,-1);
	set_trit(&cp,3,-1);
	for(cc=0;cc<27;cc++) {
		if( trs_to_digit(&cp) != 12 || trs_to_digit(&cp) != 11 ) {
			electrified_typewriter(cp,0);
		}
		inc_trs(&cp);
	}

	set_trit(&cp,1,1);
	set_trit(&cp,2,1);
	set_trit(&cp,3,-1);
	electrified_typewriter(cp,0);

	set_trit(&cp,1,-1);
	set_trit(&cp,2,-1);
	set_trit(&cp,3,-1);
	for(cc=0;cc<27;cc++) {
		if( trs_to_digit(&cp) != 12 || trs_to_digit(&cp) != 11 ) {
			electrified_typewriter(cp,0);
		}
		inc_trs(&cp);
	}

	set_trit(&cp,1,1);
	set_trit(&cp,2,1);
	set_trit(&cp,3,0);
	electrified_typewriter(cp,1);

	set_trit(&cp,1,-1);
	set_trit(&cp,2,-1);
	set_trit(&cp,3,-1);
	for(cc=0;cc<27;cc++) {
		if( trs_to_digit(&cp) != 12 || trs_to_digit(&cp) != 11 ) {
			electrified_typewriter(cp,1);
		}
		inc_trs(&cp);
	}

	set_trit(&cp,1,1);
	set_trit(&cp,2,1);
	set_trit(&cp,3,-1);
	electrified_typewriter(cp,1);

	set_trit(&cp,1,-1);
	set_trit(&cp,2,-1);
	set_trit(&cp,3,-1);
	for(cc=0;cc<27;cc++) {
		if( trs_to_digit(&cp) != 12 || trs_to_digit(&cp) != 11 ) {
			electrified_typewriter(cp,1);
		}
		inc_trs(&cp);
	}

	printf("\nt15 --- inc_trs()\n");

	//uint8_t cmd[20];
	//trs_t dst;
	//dst.l = 9;
	//dst.tb = 0;
	trs_t inr;
	inr.l = 5;
	set_trit(&inr,1,-1);
	set_trit(&inr,2,-1);
	set_trit(&inr,3,-1);
	set_trit(&inr,4,-1);
	set_trit(&inr,5,0);

	uint8_t mm;
	for(mm=1;mm<=54;mm++) {
		view_short_reg(&inr,"inr");
		inc_trs(&inr);
	}

	//dump_fram();
	//dump_fram();
	printf("\n --- Size bytes DRUM, FRAM --- \n");
	printf(" - mem_drum=%f\r\n", (float)(sizeof(mem_drum)) );
	printf(" - mem_fram=%f\r\n", (float)(sizeof(mem_fram)) );

	trs_t zd;
	zd.l = 4;
	set_trit(&zd,1,1);
	set_trit(&zd,2,1);
	set_trit(&zd,3,1);
	set_trit(&zd,4,1);
	printf(" - zone_drum_to_index()=%i\r\n",zone_drum_to_index(zd) );

	trs_t rd;
	rd.l = 4;
	set_trit(&rd,1,1);
	set_trit(&rd,2,1);
	set_trit(&rd,3,1);
	set_trit(&rd,4,1);
	printf(" - row_drum_to_index()=%i\r\n",row_drum_to_index(rd) );

	trs_t zr;
	zr.l = 1;
	set_trit(&zr,1,-1);
	printf(" - zone_fram_to_index()=%i\r\n",zone_fram_to_index(zr) );
	set_trit(&zr,1,0);
	printf(" - zone_fram_to_index()=%i\r\n",zone_fram_to_index(zr) );
	set_trit(&zr,1,1);
	printf(" - zone_fram_to_index()=%i\r\n",zone_fram_to_index(zr) );

	trs_t rr;
	rr.l = 4;

	set_trit(&rr,1,-1);
	set_trit(&rr,2,-1);
	set_trit(&rr,3,-1);
	set_trit(&rr,4,0);
	printf(" - row_fram_to_index()=%i\r\n",row_fram_to_index(rr) );

	set_trit(&rr,1,0);
	set_trit(&rr,2,0);
	set_trit(&rr,3,0);
	set_trit(&rr,4,0);
	printf(" - row_fram_to_index()=%i\r\n",row_fram_to_index(rr) );

	set_trit(&rr,1,1);
	set_trit(&rr,2,1);
	set_trit(&rr,3,1);
	set_trit(&rr,4,1);
	printf(" - row_fram_to_index()=%i\r\n",row_fram_to_index(rr) );


	printf(" - 1. \r\n");
	trs_t aa;
	aa.l = 5;
	set_trit(&aa,1,-1);
	set_trit(&aa,2,-1);
	set_trit(&aa,3,1);
	set_trit(&aa,4,1);
	set_trit(&aa,5,-1);

	R.tb = 0;
	set_trit(&R,1,-1);
	set_trit(&R,18,-1);
	view_short_reg(&R,"R");

	st_fram(aa,R);
	R = ld_fram(aa);
	view_short_reg(&aa,"aa");
	view_short_reg(&R,"R");

	printf(" - 2. \r\n");
	set_trit(&aa,1,-1);
	set_trit(&aa,2,-1);
	set_trit(&aa,3,-1);
	set_trit(&aa,4,-1);
	set_trit(&aa,5,-1);

	st_fram(aa,aa);
	R = ld_fram(aa);
	view_short_reg(&aa,"aa");
	view_short_reg(&R,"R");

	printf(" - 3. \r\n");
	inc_trs(&aa);
	st_fram(aa,aa);
	R = ld_fram(aa);
	view_short_reg(&aa,"aa");
	view_short_reg(&R,"R");

	printf(" - 4. \r\n");
	inc_trs(&aa);
	st_fram(aa,aa);
	R = ld_fram(aa);
	view_short_reg(&aa,"aa");
	view_short_reg(&R,"R");

	set_trit(&aa,1,1);
	set_trit(&aa,2,1);
	set_trit(&aa,3,1);
	set_trit(&aa,4,1);
	set_trit(&aa,5,1);
	st_fram(aa,aa);
	R = ld_fram(aa);
	view_short_reg(&aa,"aa");
	view_short_reg(&R,"R");

	//
	//dump_fram();
	//dump_drum();

	printf("\r\n --- smtr() --- \n");
    R = smtr("-+0+-");
	view_short_reg(&R,"R");

    R = smtr("---------");
	view_short_reg(&R,"R");

    R = smtr("+++++++++");
	view_short_reg(&R,"R");
	
	
	printf("\n\nt17 --- trishort, trilong types data\n");
	
	static trishort ts = (trishort)1<<31;
	static trilong  tl = (trilong)1<<63;
	
	printf("sizeof trishort ts=%li\r\n",sizeof(ts)); 
	printf("sizeof trilong  tl=%li\r\n",sizeof(tl)); 

	for(uint8_t i=0;i<32;i++) {
		printf(" tl=%016p\r\n",tl); 
		printf(" ts=tl=%08p\r\n",ts); 
		printf("\ntl >>= 2\n"); 
		tl >>=2;
		ts = tl;
	}


	printf("\n --- STOP Triniti tests VM SETUN-1958 ---\n");
}


/** *********************************************
 *  Тестирование виртуальной машины "Сетунь-1958"
 *  типов данных, функции
 *  ---------------------------------------------
 */
void Setun_test_Opers( void ) {
	
	trs_t exK;	
	trs_t oper;
	trs_t m0;
	trs_t m1;
	trs_t addr;
	trs_t ad1;
	trs_t ad2;
	uint8_t ret_exec;
	
	//t__ test sub_trs
	trs_t aaa = smtr("+++++++++000000000");
	trs_t bbb = smtr("---------+++++++++");
	trs_t ccc;
	ccc = sub_trs(aaa,bbb);
	view_short_reg(&ccc,"c=a-b");

	//t18 test Oper=k6..8[+00] : (A*)=>(S)
	printf("\nt18: test Oper=k6..8[+00] : (A*)=>(S)\n");	
	
	reset_setun_1958(); 
		
	addr = smtr("00000");
	view_short_reg(&addr,"addr=");
	m0 = smtr("+0-0+0-00"); 	
	st_fram(addr,m0); 
	view_fram(addr);		
	
	addr = smtr("0000+"); 
	m1 = smtr("00000+000"); 
	st_fram(addr,m1); 
	view_fram(addr);		
				
	/* Begin address fram */ 	
	C = smtr("0000+");	
	
	printf("\nreg C = 00001\n"); 
	
	/** 
	* work VM Setun-1958
	*/

	K = ld_fram(C);
	view_short_reg(&K,"K=");
	exK = control_trs(K);	
	oper = slice_trs(K,6,8);
	ret_exec = execute_trs(exK,oper);
	printf("ret_exec = %i\r\n",ret_exec);
	printf("\n");
	
	view_short_regs();

	
	//t19 test Oper=k6..8[+00] : (A*)=>(S)
	printf("\nt19: test Oper=k6..8[+00] : (A*)=>(S)\n");	
	
	reset_setun_1958(); 

	F = smtr("000++"); 	
		
	addr = smtr("000++");
	view_short_reg(&addr,"addr=");
	m0 = smtr("0++++++++"); 	
	st_fram(addr,m0); 

	addr = smtr("000--");
	view_short_reg(&addr,"addr=");
	m0 = smtr("0++++++++"); 	
	st_fram(addr,m0); 

	addr = smtr("000-0");
	view_short_reg(&addr,"addr=");
	m0 = smtr("0--------"); 	
	st_fram(addr,m0); 

	addr = smtr("00000");
	view_short_reg(&addr,"addr=");
	m0 = smtr("+0-0+0-00"); 	
	st_fram(addr,m0); 
	
	addr = smtr("0000+"); 
	m1 = smtr("00000+0-0"); 
	st_fram(addr,m1); 

	/* Begin address fram */ 	
	C = smtr("0000+");	
	
	printf("\nreg C = 00001\n"); 
	
	/** 
	* work VM Setun-1958
	*/
	K = ld_fram(C);	
	exK = control_trs(K);	
	view_short_reg(&K,"K=");
	oper = slice_trs(K,6,8);
	ret_exec = execute_trs(exK,oper);
	printf("ret_exec = %i\r\n",ret_exec);
	printf("\n");


	addr = smtr("0000+"); 
	m1 = smtr("00000+00+"); 
	st_fram(addr,m1); 

	/* Begin address fram */ 	
	C = smtr("0000+");	
	
	printf("\nreg C = 00001\n"); 
	
	/** 
	* work VM Setun-1958
	*/
	K = ld_fram(C);	
	exK = control_trs(K);	
	view_short_reg(&K,"K=");
	oper = slice_trs(K,6,8);
	ret_exec = execute_trs(exK,oper);
	printf("ret_exec = %i\r\n",ret_exec);
	printf("\n");

	view_short_regs();
	
	/* Begin address fram */ 	
	C = smtr("000+0");	
	printf("\nreg C = 00010\n"); 
	
	addr = smtr("000+0"); 
	m1 = smtr("00000+0--"); 
	st_fram(addr,m1); 

	/** 
	* work VM Setun-1958
	*/	
	K = ld_fram(C);	
	exK = control_trs(K);	
	view_short_reg(&K,"K=");
	oper = slice_trs(K,6,8);
	ret_exec = execute_trs(exK,oper);
	printf("ret_exec = %i\r\n",ret_exec);
	printf("\n");

	ad1 = smtr("000-0");
	ad2 = smtr("00+-0");	
	dumpf(ad1,ad2);

	view_short_regs();

	/* Begin address fram */ 	
	C = smtr("000++");	
	printf("\nreg C = 00011\n"); 
	
	addr = smtr("000++"); 
	m1 = smtr("00000++00"); 
	st_fram(addr,m1); 

	/** 
	* work VM Setun-1958
	*/	
	K = ld_fram(C);	
	exK = control_trs(K);	
	view_short_reg(&K,"K=");
	oper = slice_trs(K,6,8);
	ret_exec = execute_trs(exK,oper);
	printf("ret_exec = %i\r\n",ret_exec);
	printf("\n");

	ad1 = smtr("000-0");
	ad2 = smtr("00+-0");	
	dumpf(ad1,ad2);

	view_short_regs();

}	

/** -------------------------------
 *  Main
 *  -------------------------------
 */
int main ( int argc, char *argv[] )
{
	FILE *file;
	//trs_t k;
	trs_t inr;
	//addr pM;

	uint8_t cmd[20]; 
	trs_t dst;
	
	//trs_t sc;
	//trs_t ka;
	trs_t addr;
	trs_t oper;
	uint8_t ret_exec;


#if (TRI_TEST == 1)
	/* Выполнить тесты */
	Triniti_tests();	
#endif

	Setun_test_Opers();
	
	return 0;


	printf("\r\n --- EMULATOR SETUN-1958 --- \r\n");		

	/* Сброс виртуальной машины "Сетунь-1958" */
	printf("\r\n --- Reset Setun-1958 --- \r\n");		
	reset_setun_1958(); 	
	view_short_regs();


	/**
	 * Загрузить из файла тест-программу
	 */
	printf("\r\n --- Load 'ur0/01-test.txs' --- \r\n");		
    inr = smtr("----0"); /* cчетчик адреса коротких слов */
	dst.l = 9;
	MR.l = 18;
	file = fopen("ur0/01-test.txs", "r");
	while (fscanf (file, "%s\r\n", cmd) != EOF) {		
		printf("%s -> ", cmd);
		cmd_str_2_trs(cmd,&dst);
		view_short_reg(&inr," addr");
		st_fram(inr,dst);
		inr = next_address(inr);				
	}
	printf(" --- EOF 'test-1.txs' --- \r\n\r\n");

	dump_fram(); // test Ok'

	//dump_drum(); //TODO dbg

	/**
	 * Выполнить первый код "Сетунь-1958"
	 */
	printf("\r\n[ Start Setun-1958 ]\r\n");

	/**
	 * Выполение программы в ферритовой памяти "Сетунь-1958"
	 */
	
	/* Begin address fram */ 
	C = smtr("0000+");	    
	printf("\nreg C = 00001\n"); 
	
	/** 
	* work VM Setun-1958
	*/
	for(uint16_t jj=1;jj<10000;jj++) {
		
		K = ld_fram(C);
		addr = control_trs(K);
		oper = slice_trs(K,6,8);
				
		ret_exec = execute_trs(addr,oper);
		
		if( (ret_exec == STOP_DONE) ||
			(ret_exec == STOP_OVER) ||
			(ret_exec == STOP_ERROR) 
		  ) {	
			break;
		}		
	}
	printf("\n");
	printf(" - ret_exec = %i\r\n",ret_exec);
	printf(" - opers    = %d\r\n",10000);
	
	printf("\r\n[ Stop Setun-1958 ]\r\n");

} /* 'main.c' */

/* EOF 'setun_core.c' */
