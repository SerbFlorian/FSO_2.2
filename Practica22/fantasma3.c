#include "estructura.h"

/* funcio per realitzar la carrega dels parametres de joc emmagatzemats */
/* dins d'un fitxer de text, el nom del qual es passa per referencia a  */
/* 'nom_fit'; si es detecta algun problema, la funcio avorta l'execucio */
/* enviant un missatge per la sortida d'error i retornant el codi per-	*/
/* tinent al SO (segons comentaris al principi del programa).		    */

/* funcio per moure un fantasma una posicio; retorna 1 si el fantasma   */
/* captura al menjacocos, 0 altrament					*/

int main(int n_args, char *ll_args[])
{
    objecte seg;
    int k, vk, nd, vd[3];
    int p;

    id_retard = atoi(ll_args[4]);
    retard = map_mem(id_retard);

    //setbuf(stdout, NULL);
    //srand(getpid());

    id_fi1 = atoi(ll_args[2]);
    fi1 = map_mem(id_fi1);

    //setbuf(stdout, NULL);
    //srand(getpid());

    id_fi2 = atoi(ll_args[3]);
    fi2 = map_mem(id_fi2);

    //setbuf(stdout, NULL);
    //srand(getpid());

    n_fil1 = atoi(ll_args[2]);
    n_col = atoi(ll_args[2]);
    win_set(p_win, n_fil1, n_col);
    fprintf(stderr,"\n%d\n", id_win);
    p = 0;

    setbuf(stdout, NULL);
    srand(getpid());

    do
    {
	win_retard(*retard);
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
		seg.f = f1[w].f + df[vk]; /* calcular posicio en la nova dir.*/
                seg.c = f1[w].c + dc[vk];
		seg.a = win_quincar(seg.f, seg.c); /* calcular caracter seguent posicio */
                if ((seg.a == ' ') || (seg.a == '.') || (seg.a == '0'))
                {
                    vd[nd] = vk; /* memoritza com a direccio possible */
                    nd++;
                }
            }
            if (nd == 0)               /* si no pot continuar, */
            {
		f1[w].d = (f1[w].d + 2) % 4; /* canvia totalment de sentit */
            }
            else
            {
                if (nd == 1)                /* si nomes pot en una direccio*/
	        {
		    f1[w].d = vd[0];           /* li assigna aquesta */
		}
		else                        /* altrament */
                {
		    f1[w].d = vd[rand() % nd]; /* segueix una dir. aleatoria */
		}
                seg.f = f1[w].f + df[f1[w].d]; /* calcular seguent posicio final */
                seg.c = f1[w].c + dc[f1[w].d];
		seg.a = win_quincar(seg.f, seg.c);      /* calcular caracter seguent posicio */
		win_escricar(f1[w].f, f1[w].c, f1[w].a, NO_INV); /* esborra posicio anterior */
		f1[w].f = seg.f;
                f1[w].c = seg.c;
                f1[w].a = seg.a;                          /* actualitza posicio */
		win_escricar(f1[w].f, f1[w].c, '1', NO_INV); /* redibuixa fantasma */
                if (f1[w].a == '0')
		{
                    *fi1 = 1; /* ha capturat menjacocos */
		}
	    }
            }
        }
    } while (!*fi1 && !*fi2);
}
