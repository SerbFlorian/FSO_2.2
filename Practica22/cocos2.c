
#define _REENTRANT
#include <stdio.h>  /* incloure definicions de funcions estandard */
#include <stdlib.h> /* per exit() */
#include <stdint.h>
#include <string.h>
#include <unistd.h>    /* per getpid() */
#include "winsuport.h" /* incloure definicions de funcions propies */
#include <pthread.h>
#include <time.h>

#define MIN_FIL 7 /* definir limits de variables globals */
#define MAX_FIL 25
#define MIN_COL 10
#define MAX_COL 80

#define MAX_THREADS 10 /*Declarem la constant maxima de threads*/
#define MAX_FAN (MAX_THREADS - 1)
/* definir estructures d'informacio */
typedef struct
{            /* per un objecte (menjacocos o fantasma) */
    int f;   /* posicio actual: fila */
    int c;   /* posicio actual: columna */
    int d;   /* direccio actual: [0..3] */
    float r; /* per indicar un retard relati */
    char a;  /* caracter anterior en pos. actual */
} objecte;

pthread_t tid[MAX_THREADS]; /*taula d'identificadors dels threads*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* variables globals */
int n_fil1, n_col; /* dimensions del camp de joc */
char tauler[70];   /* nom del fitxer amb el laberint de joc */
char c_req;        /* caracter de pared del laberint */

objecte mc; /* informacio del menjacocos */
objecte f1 [MAX_FAN]; /* informacio del fantasma 1 */

int df[] = {-1, 0, 1, 0}; /* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1}; /* dalt, esquerra, baix, dreta */

int cocos;  /* numero restant de cocos per menjar */
int retard; /* valor del retard de moviment, en mil.lisegons */
int fi1, fi2, p;
int num_fan;

/* funcio per realitzar la carrega dels parametres de joc emmagatzemats */
/* dins d'un fitxer de text, el nom del qual es passa per referencia a  */
/* 'nom_fit'; si es detecta algun problema, la funcio avorta l'execucio */
/* enviant un missatge per la sortida d'error i retornant el codi per-	*/
/* tinent al SO (segons comentaris al principi del programa).		    */
void carrega_parametres(const char *nom_fit)
{
    int i = 1;
    FILE *fit;

    fit = fopen(nom_fit, "rt"); /* intenta obrir fitxer */
    if (fit == NULL)
    {
        fprintf(stderr, "No s'ha pogut obrir el fitxer \'%s\'\n", nom_fit);
        exit(2);
    }

    if (!feof(fit))
        fscanf(fit, "%d %d %s %c\n", &n_fil1, &n_col, tauler, &c_req);
    else
    {
        fprintf(stderr, "Falten parametres al fitxer \'%s\'\n", nom_fit);
        fclose(fit);
        exit(2);
    }
    if ((n_fil1 < MIN_FIL) || (n_fil1 > MAX_FIL) ||
        (n_col < MIN_COL) || (n_col > MAX_COL))
    {
        fprintf(stderr, "Error: dimensions del camp de joc incorrectes:\n");
        fprintf(stderr, "\t%d =< n_fil1 (%d) =< %d\n", MIN_FIL, n_fil1, MAX_FIL);
        fprintf(stderr, "\t%d =< n_col (%d) =< %d\n", MIN_COL, n_col, MAX_COL);
        fclose(fit);
        exit(3);
    }

    if (!feof(fit))
        fscanf(fit, "%d %d %d %f\n", &mc.f, &mc.c, &mc.d, &mc.r);
    else
    {
        fprintf(stderr, "Falten parametres al fitxer \'%s\'\n", nom_fit);
        fclose(fit);
        exit(2);
    }
    if ((mc.f < 1) || (mc.f > n_fil1 - 3) ||
        (mc.c < 1) || (mc.c > n_col - 2) ||
        (mc.d < 0) || (mc.d > 3))
    {
        fprintf(stderr, "Error: parametres menjacocos incorrectes:\n");
        fprintf(stderr, "\t1 =< mc.f (%d) =< n_fil1-3 (%d)\n", mc.f, (n_fil1 - 3));
        fprintf(stderr, "\t1 =< mc.c (%d) =< n_col-2 (%d)\n", mc.c, (n_col - 2));
        fprintf(stderr, "\t0 =< mc.d (%d) =< 3\n", mc.d);
        fclose(fit);
        exit(4);
    }
    fscanf(fit, "%d %d %d %f\n", &f1[0].f, &f1[0].c, &f1[0].d, &f1[0].r);
    while (!feof(fit))
    {
        fscanf(fit, "%d %d %d %f\n", &f1[i].f, &f1[i].c, &f1[i].d, &f1[i].r);
    	i++;
    }
    num_fan = i;
    /*if (feof(fit))
    {
        fprintf(stderr, "Falten parametres al fitxer \'%s\'\n", nom_fit);
        fclose(fit);
        exit(2);
    }*/
    i = 1;
    if ((f1[i].f < 1) || (f1[i].f > n_fil1 - 3) ||
        (f1[i].c < 1) || (f1[i].c > n_col - 2) ||
        (f1[i].d < 0) || (f1[i].d > 3))
    {
        fprintf(stderr, "Error: parametres fantasma 1 incorrectes:\n");
        fprintf(stderr, "\t1 =< f1.f (%d) =< n_fil1-3 (%d)\n", f1[i].f, (n_fil1 - 3));
        fprintf(stderr, "\t1 =< f1.c (%d) =< n_col-2 (%d)\n", f1[i].c, (n_col - 2));
        fprintf(stderr, "\t0 =< f1.d (%d) =< 3\n", f1[i].d);
        fclose(fit);
        exit(5);
    }
    fclose(fit); /* fitxer carregat: tot OK! */
    printf("Joc del MenjaCocos\n\tTecles: \'%c\', \'%c\', \'%c\', \'%c\', RETURN-> sortir\n",
           TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
    printf("prem una tecla per continuar:\n");
    getchar();
}

/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
void inicialitza_joc(void)
{
    int r, i, j, w;
    char strin[12];

    r = win_carregatauler(tauler, n_fil1 - 1, n_col, c_req);
    if (r == 0)
    {
        mc.a = win_quincar(mc.f, mc.c);
        if (mc.a == c_req)
            r = -6; /* error: menjacocos sobre pared */
        else
        {
            for (w = 0; w < num_fan; w++)
	    {
            f1[w].a = win_quincar(f1[w].f, f1[w].c);
            if (f1[w].a == c_req)
                r = -7; /* error: fantasma sobre pared */
            else
            {
                cocos = 0; /* compta el numero total de cocos */
                for (i = 0; i < n_fil1 - 1; i++)
                    for (j = 0; j < n_col; j++)
                        if (win_quincar(i, j) == '.')
                            cocos++;
                win_escricar(mc.f, mc.c, '0', NO_INV);
                win_escricar(f1[w].f, f1[w].c, '1', NO_INV);

                if (mc.a == '.')
                    cocos--; /* menja primer coco */

                sprintf(strin, "Cocos: %d", cocos);
                win_escristr(strin);
            }
            }
        }
    }
    if (r != 0)
    {
        win_fi();
        fprintf(stderr, "Error: no s'ha pogut inicialitzar el joc:\n");
        switch (r)
        {
        case -1:
            fprintf(stderr, "  nom de fitxer erroni\n");
            break;
        case -2:
            fprintf(stderr, "  numero de columnes d'alguna fila no coincideix amb l'amplada del tauler de joc\n");
            break;
        case -3:
            fprintf(stderr, "  numero de columnes del laberint incorrecte\n");
            break;
        case -4:
            fprintf(stderr, "  numero de files del laberint incorrecte\n");
            break;
        case -5:
            fprintf(stderr, "  finestra de camp de joc no oberta\n");
            break;
        case -6:
            fprintf(stderr, "  posicio inicial del menjacocos damunt la pared del laberint\n");
            break;
        case -7:
            fprintf(stderr, "  posicio inicial del fantasma damunt la pared del laberint\n");
            break;
        }
        exit(7);
    }
}

/* funcio per moure un fantasma una posicio; retorna 1 si el fantasma   */
/* captura al menjacocos, 0 altrament					*/
void *mou_fantasma(void *index)
{
    objecte seg;
    int ret;
    int k, vk, nd, vd[3];

    ret = 0;
    do
    {
	win_retard(retard);
        if ((p % 2) == 0)
        {
	    for (int w = 0; w < num_fan; w++)
            {
	    nd = 0;
            for (k = -1; k <= 1; k++) /* provar direccio actual i dir. veines */
            {
                vk = (f1[w].d + k) % 4; /* direccio veina */
                if (vk < 0)
		{
                    vk += 4; /* corregeix negatius */
                }
		pthread_mutex_lock(&mutex);
		seg.f = f1[w].f + df[vk]; /* calcular posicio en la nova dir.*/
                seg.c = f1[w].c + dc[vk];
		pthread_mutex_unlock(&mutex);
		pthread_mutex_lock(&mutex);
		seg.a = win_quincar(seg.f, seg.c); /* calcular caracter seguent posicio */
		pthread_mutex_unlock(&mutex);
                if ((seg.a == ' ') || (seg.a == '.') || (seg.a == '0'))
                {
                    vd[nd] = vk; /* memoritza com a direccio possible */
                    nd++;
                }
            }
            if (nd == 0)               /* si no pot continuar, */
            {
		pthread_mutex_lock(&mutex);
		f1[w].d = (f1[w].d + 2) % 4; /* canvia totalment de sentit */
		pthread_mutex_unlock(&mutex);
            }
            else
            {
                if (nd == 1)                /* si nomes pot en una direccio */
		{
		    pthread_mutex_lock(&mutex);
		    f1[w].d = vd[0];           /* li assigna aquesta */
                    pthread_mutex_unlock(&mutex);
		}
		else                        /* altrament */
                {
		    pthread_mutex_lock(&mutex);
		    f1[w].d = vd[rand() % nd]; /* segueix una dir. aleatoria */
		    pthread_mutex_unlock(&mutex);
		}
		pthread_mutex_lock(&mutex);
                seg.f = f1[w].f + df[f1[w].d]; /* calcular seguent posicio final */
                seg.c = f1[w].c + dc[f1[w].d];
                pthread_mutex_unlock(&mutex);
		pthread_mutex_lock(&mutex);
		seg.a = win_quincar(seg.f, seg.c);      /* calcular caracter seguent posicio */
		win_escricar(f1[w].f, f1[w].c, f1[w].a, NO_INV); /* esborra posicio anterior */
		pthread_mutex_unlock(&mutex);
		pthread_mutex_lock(&mutex);
		f1[w].f = seg.f;
                f1[w].c = seg.c;
                f1[w].a = seg.a;                          /* actualitza posicio */
		win_escricar(f1[w].f, f1[w].c, '1', NO_INV); /* redibuixa fantasma */
                pthread_mutex_unlock(&mutex);
                if (f1[w].a == '0')
		{
		    pthread_mutex_lock(&mutex);
                    fi1 = 1; /* ha capturat menjacocos */
                    pthread_mutex_unlock(&mutex);
		}
	    }
            }
        }
    } while (!fi1 && !fi2);
}

/* funcio per moure el menjacocos una posicio, en funcio de la direccio de   */
/* moviment actual; retorna -1 si s'ha premut RETURN, 1 si s'ha menjat tots  */
/* els cocos, i 0 altrament */
void *mou_menjacocos(void *null)
{
    char strin[12];
    objecte seg;
    int tec, ret;

    ret = 0;

    do
    {
	win_retard(retard);
        pthread_mutex_lock(&mutex);
        tec = win_gettec();
        pthread_mutex_unlock(&mutex);
        if (tec != 0)
        {
            switch (tec) /* modificar direccio menjacocos segons tecla */
            {
            case TEC_AMUNT:
                mc.d = 0;
		break;
            case TEC_ESQUER:
                mc.d = 1;
                break;
            case TEC_AVALL:
                mc.d = 2;
                break;
            case TEC_DRETA:
                mc.d = 3;
                break;
            case TEC_RETURN:
                ret = -1;
                break;
	    }
        }

	fi1 = ret;

	pthread_mutex_lock(&mutex);
	seg.f = mc.f + df[mc.d]; /* calcular seguent posicio */
        seg.c = mc.c + dc[mc.d];
	seg.a = win_quincar(seg.f, seg.c); /* calcular caracter seguent posicio */
        pthread_mutex_unlock(&mutex);
	if ((seg.a == ' ') || (seg.a == '.'))
        {
            pthread_mutex_lock(&mutex);
            win_escricar(mc.f, mc.c, ' ', NO_INV); /* esborra posicio anterior */
            pthread_mutex_unlock(&mutex);
	    pthread_mutex_lock(&mutex);
	    mc.f = seg.f;
            mc.c = seg.c;                          /* actualitza posicio */
	    win_escricar(mc.f, mc.c, '0', NO_INV); /* redibuixa menjacocos */
            pthread_mutex_unlock(&mutex);
	    if (seg.a == '.')
            {
                cocos--;
                sprintf(strin, "Cocos: %d", cocos);
		win_escristr(strin);
                if (cocos == 0)
		{
                    fi2 = 1;
	        }
	    }
        }
    p++;
    }while (!fi1 && !fi2);
}

/* programa principal				    */
int main(int n_args, const char *ll_args[])
{
    int temps_total = 0;
    time_t begin = time(NULL);
    int t, t_total, rc, n; /* variables locals */

    pthread_mutex_init(&mutex, NULL);

    srand(getpid()); /* inicialitza numeros aleatoris */

    if ((n_args != 2) && (n_args != 3))
    {
        fprintf(stderr, "Comanda: cocos0 fit_param [retard]\n");
        exit(1);
    }
    carrega_parametres(ll_args[1]);

    if (n_args == 3)
        retard = atoi(ll_args[2]);
    else
        retard = 100;

    rc = win_ini(&n_fil1, &n_col, '+', INVERS); /* intenta crear taulell */
    if (rc == 0)                                /* si aconsegueix accedir a l'entorn CURSES */
    {
        inicialitza_joc();
        p = 0;
        fi1 = 0;
        fi2 = 0;
	win_retard(retard);

        pthread_create(&tid[0], NULL, mou_menjacocos, (void *)(intptr_t)0);
        pthread_mutex_init(&mutex, NULL);
        pthread_create(&tid[0], NULL, mou_fantasma, (void *)(intptr_t)0);

        for (int i = 0; i <= num_fan; i++)
        {
            pthread_join(tid[i], (void **)&t);
            t_total += t;
        }

        win_fi();

        if (fi1 == -1)
            printf("S'ha aturat el joc amb tecla RETURN!\n");
        else
        {
            if (fi1){
                printf("Ha guanyat l'ordinador!\n");
		printf("Han faltat %d cocos ha menjar\n", cocos);
            }
	    else
                printf("Ha guanyat l'usuari!\n");
        }
	win_retard(retard);
    }
    else
    {
        fprintf(stderr, "Error: no s'ha pogut crear el taulell:\n");
        switch (rc)
        {
        case -1:
            fprintf(stderr, "camp de joc ja creat!\n");
            break;
        case -2:
            fprintf(stderr, "no s'ha pogut inicialitzar l'entorn de curses!\n");
            break;
        case -3:
            fprintf(stderr, "les mides del camp demanades son massa grans!\n");
            break;
        case -4:
            fprintf(stderr, "no s'ha pogut crear la finestra!\n");
            break;
        }
        exit(6);
    }
    pthread_mutex_destroy(&mutex);
    time_t end = time(NULL);
    temps_total += (int)(end - begin);
    printf("El joc ha tardat %d segons\n", temps_total);
    return (0);
}
