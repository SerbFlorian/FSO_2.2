
#define _REENTRANT
#include <stdio.h>  /* incloure definicions de funcions estandard */
#include <stdlib.h> /* per exit() */
#include <stdint.h>
#include <string.h>
#include <unistd.h>    /* per getpid() */
#include "winsuport2.h" /* incloure definicions de funcions propies */
#include <pthread.h>
#include <time.h>
#include "memoria.h"
#include "semafor.h"
#include "missatge.h"

#define MIN_FIL 7 /* definir limits de variables globals */
#define MAX_FIL 25
#define MIN_COL 10
#define MAX_COL 80

#define MAX_PROCS 10 /*Declarem la constant maxima de threads*/
#define MAX_FAN (MAX_PROCS - 1)
/* definir estructures d'informacio */
typedef struct
{            /* per un objecte (menjacocos o fantasma) */
    int f;   /* posicio actual: fila */
    int c;   /* posicio actual: columna */
    int d;   /* direccio actual: [0..3] */
    float r; /* per indicar un retard relati */
    char a;  /* caracter anterior en pos. actual */
} objecte;

pid_t tid[MAX_PROCS];

/* variables globals */
int n_fil1, n_col; /* dimensions del camp de joc */
char tauler[70];   /* nom del fitxer amb el laberint de joc */
char c_req;        /* caracter de pared del laberint */

objecte mc; /* informacio del menjacocos */
//objecte f1 [MAX_FAN]; /* informacio del fantasma 1 */

//int df[] = {-1, 0, 1, 0}; /* moviments de les 4 direccions possibles */
//int dc[] = {0, -1, 0, 1}; /* dalt, esquerra, baix, dreta */

int cocos;  /* numero restant de cocos per menjar */

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

/* programa principal				    */
int main(int n_args, const char *ll_args[])
{
    int temps_total = 0;
    time_t begin = time(NULL);
    int i, t, t_total, rc, n; /* variables locals */
    int num;
    char a1[20], a2[20], a3[20], a4[20];
    int id_retard, *retard, n_retard;
    int id_fi1, *fi1, n_fi1;
    int id_fi2, *fi2, n_fi2;
    int id_dc, *dc, n_dc;
    int id_df, *df, n_df;
    int id_num_fan, *num_fan, n_num_fan;
    int id_p, *p, n_p;
    int id_f1, *f1, n_f1;
    int id_sem, id_bustia;


    //id_fantasmes = ini_mem(sizeof(int));  /* crear zona mem. compartida */
    //p_fantasmes = map_mem(id_fantasmes);  /* obtenir adreça mem. compartida */
    //*p_fantasmes = n_fantasmes;           /* inicialitza variabel compartida */
    //sprintf(a2, "%i", id_fantasmes);         /* convertir id. memoria en string */

    id_retard = ini_mem(sizeof(int));
    retard = map_mem(id_retard);
    *retard = n_retard;
    sprintf(a2, "%i", id_retard);

    id_fi1 = ini_mem(sizeof(int));
    fi1 = map_mem(id_fi1);
    *fi1 = n_fi1;
    sprintf(a2, "%i", id_fi1);

    id_fi2 = ini_mem(sizeof(int));
    fi2 = map_mem(id_fi2);
    *fi2 = n_fi2;
    sprintf(a2, "%i", id_fi2);

    id_dc = ini_mem(sizeof(4));
    dc = map_mem(id_dc);
    *dc = n_dc;
    sprintf(a2, "%i", id_dc);

    id_df = ini_mem(sizeof(4));
    df = map_mem(id_df);
    *df = n_df;
    sprintf(a2, "%i", id_df);

    id_num_fan = ini_mem(sizeof(int));
    num_fan = map_mem(id_num_fan);
    *num_fan = n_num_fan;
    sprintf(a2, "%i", id_num_fan);

    id_p = ini_mem(sizeof(int));
    p = map_mem(id_p);
    *p = n_p;
    sprintf(a2, "%i", id_p);

    id_f1 = ini_mem(sizeof(MAX_FAN));
    f1 = map_mem(id_f1);
    *f1 = n_f1;
    sprintf(a2, "%i", id_f1);

    id_sem = ini_sem(1);		/* Crear semafor IPC inicialment obert */
    sprintf(a3, "%i", id_sem);		/* Convertir identificador sem. en string */

    id_bustia = ini_mis();		/* Crear bustia IPC */
    sprintf(a4, "%i", id_bustia);	/* Convertir indetificador bustia en string */

    //pthread_mutex_init(&mutex, NULL);

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
	num = 0;
	for (i = 0; i < num_fan; i++)
	{
		tid[num] = fork();      /* crea un nou proces */
        	if (tid[num] == (pid_t) 0)
        	{
                	sprintf(a1, "%i", (i+1));
			execlp("./mou_fantasmes", "mou_fantasmes", a1, ll_args[2], a2, a3, a4, (char *)0);
			exit(0);
        	}
		else if (tid[num] > 0) num++;
	}
	//pthread_create(&tid[0], NULL, mou_menjacocos, (void *)(intptr_t)0);
        num = 0;
	//pthread_mutex_init(&mutex, NULL);
	for (i = 0; i < 1; i++)
        {
                tid[num] = fork();      /* crea un nou proces */
                if (tid[num] == (pid_t) 0)
                {
                        sprintf(a1, "%i", (i+1));
                        execlp("./mou_menjacocos", "mou_menjacocos", a1, ll_args[2], a2, a3, a4, (char *)0);
                        exit(0);
                }
                else if (tid[num] > 0) num++;
        }

	//pthread_create(&tid[1], NULL, mou_fantasma, (void *)(intptr_t)0);

        for (i = 0; i <= num_fan; i++)
        {
            waitpid(tid[i], &t,NULL);
	    //pthread_join(tid[i], (void **)&t);
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

    for (i = 0; i < t_total; i++)
    {
    	receiveM(id_bustia,a1);			/* treu un missate (string) de la bustia */
    }

    //pthread_mutex_destroy(&mutex);
    elim_mis(id_bustia);
    elim_sem(id_sem);
    elim_mem(id_retard);		/* Eliminem zona memoria compartida */
    elim_mem(id_fi1);
    elim_mem(id_fi2);
    elim_mem(id_dc);
    elim_mem(id_df);
    elim_mem(id_f1);
    elim_mem(id_num_fan);
    elim_mem(id_p);
    time_t end = time(NULL);
    temps_total += (int)(end - begin);
    printf("El joc ha tardat %d segons\n", temps_total);
    return (0);
}
