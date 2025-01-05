#include "common.h"

static sem_t wejscie1, wejscie2;
static pthread_mutex_t liczba_pszczol_ul_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_dostepne_miejsce = PTHREAD_COND_INITIALIZER;
//static pthread_cond_t cond_wejscie_otwarte = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t wejscie_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t blokada_ula = PTHREAD_MUTEX_INITIALIZER;

//static int wejscie_otwarte = 1;
static Stan_Ula* stan_ula_local = NULL; // wskaznik na lokalną kopie stanu ula (ten proc)

void* robotnica(void* arg)
{
    Argumenty_Watku* argumenty_watku = (Argumenty_Watku*) arg;
    if (!argumenty_watku) {
        fprintf(stderr, "[ROBOTNICA] Błąd: argumenty wątku są NULL.\n");
        pthread_exit(NULL);
    }

    Pszczola* pszczola = argumenty_watku->pszczola;
    Stan_Ula* stan_ula_dzielony = argumenty_watku->stan_ula_do_przekazania;
    int sem_id = argumenty_watku -> sem_id;

    struct sembuf lock   = {0, -1, 0};
    struct sembuf unlock = {0,  1, 0};

    while (1) {

        if(pszczola->pszczola_jest_w_ulu == 0)
        {
        pthread_mutex_lock(&liczba_pszczol_ul_mutex);
        while (stan_ula_local->obecna_liczba_pszczol_ul >= (POCZATKOWA_ILOSC_PSZCZOL / 2)) 
        {
            pthread_cond_wait(&cond_dostepne_miejsce, &liczba_pszczol_ul_mutex);
        }
        stan_ula_local->obecna_liczba_pszczol_ul++;
        pthread_mutex_unlock(&liczba_pszczol_ul_mutex);
        char* info;
            int wejscie = rand() % 2;
            if (wejscie == 1) {
                sem_wait(&wejscie1);
                sleep(1);
                info = "[ROBOTNICA] Pszczoła weszła wejsciem 1";
                sem_post(&wejscie1);
            } else {
                sem_wait(&wejscie2);
                sleep(1);
                info = "[ROBOTNICA] Pszczoła weszła wejsciem 2";
                sem_post(&wejscie2);
            }

            pszczola->pszczola_jest_w_ulu = 1;
            printf("%s oraz id: %lu\n", info, (unsigned long)pthread_self());
            printf("Obecna liczba pszczol w ulu: %d\n",  stan_ula_local->obecna_liczba_pszczol_ul);

            // Zapis do pamięci dzielonej
            if (semop(sem_id, &lock, 1) == -1) {
                perror("[ROBOTNICA] semop lock (wejscie)");
            }
            stan_ula_dzielony->obecna_liczba_pszczol_ul = stan_ula_local->obecna_liczba_pszczol_ul;
            if (semop(sem_id, &unlock, 1) == -1) {
                perror("[ROBOTNICA] semop unlock (wejscie)");
            }
            pthread_mutex_unlock(&liczba_pszczol_ul_mutex);

            //praca w ulu
            sleep(rand() % 5 + 2);
        }
        if(pszczola->pszczola_jest_w_ulu == 1)
        {
            char* info;
            int wyjscie = rand() % 2;
            if (wyjscie == 1) {
                sem_wait(&wejscie1);
                info = "[ROBOTNICA] Pszczola wyszla wejsciem 1";
                sem_post(&wejscie1);
            } else {
                sem_wait(&wejscie2);
                info = "[ROBOTNICA] Pszczola wyszla wejsciem 2";
                sem_post(&wejscie2);
            }

            pszczola->pszczola_jest_w_ulu = 0;
            pthread_mutex_lock(&liczba_pszczol_ul_mutex);
            stan_ula_local->obecna_liczba_pszczol_ul--;
            pthread_cond_signal(&cond_dostepne_miejsce);
            pthread_mutex_unlock(&liczba_pszczol_ul_mutex);
            printf("%s oraz id: %lu\n", info, (unsigned long)pthread_self());
            printf("Obecna liczba pszczol w ulu: %d\n", stan_ula_local->obecna_liczba_pszczol_ul);

            //zapis do pam.dzielonej - upd
            if (semop(sem_id, &lock, 1) == -1) {
                perror("[ROBOTNICA] semop lock (wyjście)");
            }
            stan_ula_dzielony->obecna_liczba_pszczol_ul = stan_ula_local->obecna_liczba_pszczol_ul;
            if (semop(sem_id, &unlock, 1) == -1) {
                perror("[ROBOTNICA] semop unlock (wyjście)");
            }

            pszczola->licznik_odwiedzen++;
            printf("[ROBOTNICA] Pszczoła %lu odwiedziła ul %d razy.\n", (unsigned long)pthread_self(), pszczola->licznik_odwiedzen);
                if (pszczola->licznik_odwiedzen >= pszczola->liczba_cykli) {
            printf("[ROBOTNICA] Pszczoła %lu umiera (liczba odwiedzin %d).\n", (unsigned long)pthread_self(), pszczola->licznik_odwiedzen);

            if (semop(sem_id, &lock, 1) == -1) {
                perror("[ROBOTNICA] semop lock (śmierć)");
            }
            stan_ula_local->obecna_liczba_pszczol--;
            stan_ula_dzielony->obecna_liczba_pszczol = stan_ula_local->obecna_liczba_pszczol;
            if (semop(sem_id, &unlock, 1) == -1) {
                perror("[ROBOTNICA] semop unlock (śmierć)");
            }

            free(pszczola);
            free(argumenty_watku);

            pthread_exit(NULL);
        }
        sleep(rand() % 3 + 2); //praca poza ulem
        }

        }

        /*
        // 
        pthread_mutex_lock(&liczba_pszczol_ul_mutex);
        while (stan_ula_local->obecna_liczba_pszczol_ul >= (POCZATKOWA_ILOSC_PSZCZOL / 2)) 
        {
            pthread_cond_wait(&cond_dostepne_miejsce, &liczba_pszczol_ul_mutex);
        }

        if (pszczola->pszczola_jest_w_ulu == 0) {
            char* info;
            int wejscie = rand() % 2;
            if (wejscie == 1) {
                sem_wait(&wejscie1);
                sleep(1);
                info = "[ROBOTNICA] Pszczoła weszła wejściem 1";
                sem_post(&wejscie1);
            } else {
                sem_wait(&wejscie2);
                sleep(1);
                info = "[ROBOTNICA] Pszczoła weszła wejściem 2";
                sem_post(&wejscie2);
            }

            pszczola->pszczola_jest_w_ulu = 1;
            stan_ula_local->obecna_liczba_pszczol_ul++;
            printf("%s\n", info);

            // Zapis do pamięci dzielonej
            if (semop(sem_id_global, &lock, 1) == -1) {
                perror("[ROBOTNICA] semop lock (wejście)");
            }
            stan_ula_dzielony->obecna_liczba_pszczol_ul = stan_ula_local->obecna_liczba_pszczol_ul;
            if (semop(sem_id_global, &unlock, 1) == -1) {
                perror("[ROBOTNICA] semop unlock (wejście)");
            }
        }

        if(pszczola->pszczola_jest_w_ulu == 1)
        {
        sleep(rand() % 5 + 2);
        }
        pthread_mutex_lock(&liczba_pszczol_ul_mutex);
        if (pszczola->pszczola_jest_w_ulu == 1) {
            char* info;
            int wyjscie = rand() % 2;
            if (wyjscie == 1) {
                sem_wait(&wejscie1);
                sleep(1);
                info = "[ROBOTNICA] Pszczoła wyszła wejściem 1";
                sem_post(&wejscie1);
            } else {
                sem_wait(&wejscie2);
                sleep(1);
                info = "[ROBOTNICA] Pszczoła wyszła wejściem 2";
                sem_post(&wejscie2);
            }

            pszczola->pszczola_jest_w_ulu = 0;
            stan_ula_local->obecna_liczba_pszczol_ul--;
            printf("%s\n", info);

            // Zapis do pamięci dzielonej
            if (semop(sem_id_global, &lock, 1) == -1) {
                perror("[ROBOTNICA] semop lock (wyjście)");
            }
            stan_ula_dzielony->obecna_liczba_pszczol_ul = stan_ula_local->obecna_liczba_pszczol_ul;
            if (semop(sem_id_global, &unlock, 1) == -1) {
                perror("[ROBOTNICA] semop unlock (wyjście)");
            }

            pszczola->licznik_odwiedzen++;
            printf("[ROBOTNICA] Pszczoła odwiedziła ul %d razy.\n", pszczola->licznik_odwiedzen);
            pthread_cond_signal(&cond_dostepne_miejsce);
        }
        pthread_mutex_unlock(&liczba_pszczol_ul_mutex);

        if (pszczola->licznik_odwiedzen >= pszczola->liczba_cykli) {
            printf("[ROBOTNICA] Pszczoła umiera (liczba odwiedzin %d).\n", pszczola->licznik_odwiedzen);

            if (semop(sem_id_global, &lock, 1) == -1) {
                perror("[ROBOTNICA] semop lock (śmierć)");
            }
            stan_ula_local->obecna_liczba_pszczol--;
            stan_ula_dzielony->obecna_liczba_pszczol = stan_ula_local->obecna_liczba_pszczol;
            if (semop(sem_id_global, &unlock, 1) == -1) {
                perror("[ROBOTNICA] semop unlock (śmierć)");
            }

            free(pszczola);
            free(argumenty_watku);

            pthread_exit(NULL);
        }
    }
    */
    pthread_exit(NULL);
}


void obsluga_sygnalu(int sig)
{
    if (!stan_ula_local) return;

    if (sig == SIGUSR1) {
        printf("\n[UL] Otrzymano SIGUSR1 -> zwiekszamy populacje!\n");
        stan_ula_local->maksymalna_ilosc_osobnikow *= 2;
        printf("[UL] Nowa maksymalna liczba osobnikow: %d\n", 
               stan_ula_local->maksymalna_ilosc_osobnikow);
    }
    else if (sig == SIGUSR2) {
        /*
        printf("\n[UL] Otrzymano SIGUSR2 -> zmniejszamy populacje!\n");
        stan_ula_local->obecna_liczba_pszczol /= 2;
        stan_ula_local->maksymalna_ilosc_osobnikow = 16;
        printf("[UL] Nowa maksymalna liczba osobników: %d\n", 
               stan_ula_local->maksymalna_ilosc_osobnikow);
        */
    }
}

int main(int argc, char* argv[])
{
    if (argc < 4) {
        fprintf(stderr, "[UL] Sposób użycia: %s <fd_pipe> <shm_id> <sem_id>\n", argv[0]);
        return 1;
    }


    int pipe_fd = atoi(argv[1]);
    int shm_id  = atoi(argv[2]);
    int sem_id = atoi(argv[3]); 

    sleep(1); //pszelarz musi byc pierwszy

    //fifo do wyslania pidu ula pszczelarzowi
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Nie mozna utworzyc fifo.");
            exit(EXIT_FAILURE);
        }
    }

    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("open FIFO for writing");
        exit(EXIT_FAILURE);
    }
    int pid = (int)getpid();
    char pid_str[20];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);
    printf("pid: %s\n", pid_str);
    //const char *msg = "Hello, FIFO!";
    //int ul_pid = 2;
    
    write(fifo_fd, pid_str, strlen(pid_str) + 1); 
    close(fifo_fd);
    printf("Pid ul: %s\n\n", pid_str);

    srand(time(NULL));

    // dolaczenie do pma.dzielonej
    Stan_Ula* stan_ula_dzielony = (Stan_Ula*) shmat(shm_id, NULL, 0);
    if (stan_ula_dzielony == (void*)-1) {
        perror("[UL] shmat");
        exit(EXIT_FAILURE);
    }

    stan_ula_local = (Stan_Ula*) malloc(sizeof(Stan_Ula));
    if (!stan_ula_local) {
        fprintf(stderr, "[UL] Błąd alokacji stan_ula_local.\n");
        exit(EXIT_FAILURE);
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    stan_ula_local->obecna_liczba_pszczol = 0;
    stan_ula_local->obecna_liczba_pszczol_ul = 0;
    stan_ula_local->maksymalna_ilosc_osobnikow = POCZATKOWA_ILOSC_PSZCZOL;
    stan_ula_local->stan_poczatkowy = POCZATKOWA_ILOSC_PSZCZOL;

    if (semop(sem_id, &lock, 1) == -1) {
        perror("[UL] semop lock (init)");
    }
    stan_ula_dzielony->obecna_liczba_pszczol = stan_ula_local->obecna_liczba_pszczol;
    stan_ula_dzielony->obecna_liczba_pszczol_ul = stan_ula_local->obecna_liczba_pszczol_ul;
    stan_ula_dzielony->maksymalna_ilosc_osobnikow = stan_ula_local->maksymalna_ilosc_osobnikow;
    stan_ula_dzielony->stan_poczatkowy = stan_ula_local->stan_poczatkowy;
    if (semop(sem_id, &unlock, 1) == -1) {
        perror("[UL] semop unlock (init)");
    }
    
    //init semaforow dla wejsc
    if (sem_init(&wejscie1, 0, 1) == -1) {
        perror("[UL] sem_init wejscie1");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&wejscie2, 0, 1) == -1) {
        perror("[UL] sem_init wejscie2");
        exit(EXIT_FAILURE);
    }

    // sygnaly od pszczelarza
    struct sigaction sa;
    sa.sa_handler = obsluga_sygnalu;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    printf("[UL] Start, stan początkowy: %d, max: %d, obecne pszczoły: %d\n",
           stan_ula_local->stan_poczatkowy,
           stan_ula_local->maksymalna_ilosc_osobnikow,
           stan_ula_local->obecna_liczba_pszczol);

    //tworzymy kilka statycznych robotnic
    int liczba_poczatek = POCZATKOWA_ILOSC_PSZCZOL;
    pthread_t* robotnice_tab = (pthread_t*) calloc(liczba_poczatek, sizeof(pthread_t));
    if (!robotnice_tab) {
        fprintf(stderr, "[UL] Błąd alokacji robotnice_tab.\n");
        free(stan_ula_local);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < liczba_poczatek; i++) {
        Pszczola* p = (Pszczola*) malloc(sizeof(Pszczola));
        if (!p) {
            fprintf(stderr, "[UL] Błąd alokacji pszczoły.\n");
            continue;
        }
        p->licznik_odwiedzen    = 0;
        p->liczba_cykli         = 2;
        p->pszczola_jest_w_ulu  = 0;

        stan_ula_local->obecna_liczba_pszczol++;
        if (semop(sem_id, &lock, 1) == -1) {
            perror("[UL] semop lock (tworzenie robotnic)");
        }
        stan_ula_dzielony->obecna_liczba_pszczol = stan_ula_local->obecna_liczba_pszczol;
        if (semop(sem_id, &unlock, 1) == -1) {
            perror("[UL] semop unlock (tworzenie robotnic)");
        }

        // arguemnty dla watkow
        Argumenty_Watku* args = (Argumenty_Watku*) malloc(sizeof(Argumenty_Watku));
        if (!args) {
            fprintf(stderr, "[UL] Błąd alokacji Argumenty_Watku.\n");
            free(p);
            continue;
        }
        args->pszczola                  = p;
        args->sem_id                    = sem_id;
        args->stan_ula_do_przekazania   = stan_ula_dzielony;

        pthread_create(&robotnice_tab[i], NULL, robotnica, (void*) args);
        //sleep(1);
    }
    printf("Dopiero teraz zaczynamy petle!\n");
    // petla dzialania ula
    int otrzymana_ilosc_jaj = 0;
    while (1) {
        ssize_t count = read(pipe_fd, &otrzymana_ilosc_jaj, sizeof(int));
        if (count <= 0) {
            perror("[UL] read (potok krolowej)");
            break;
        }
       // printf("[UL] Królowa zniosła %d jaj\n", otrzymana_ilosc_jaj);

        // tworzenie nowych robotnic
        if (stan_ula_local->obecna_liczba_pszczol + otrzymana_ilosc_jaj <= stan_ula_local->maksymalna_ilosc_osobnikow 
             && stan_ula_local->obecna_liczba_pszczol_ul + otrzymana_ilosc_jaj <= stan_ula_local->stan_poczatkowy / 2) {
            printf("Krolowa zniosla %d jaj\n", otrzymana_ilosc_jaj);
            pthread_t* nowe_robotnice = (pthread_t*) calloc(otrzymana_ilosc_jaj, sizeof(pthread_t));
            if (!nowe_robotnice) {
                fprintf(stderr, "[UL] Błąd alokacji dla nowych robotnic.\n");
                continue;
            }

            if (semop(sem_id, &lock, 1) == -1) {
                perror("[UL] semop lock (nowe robotnice)");
            }
                stan_ula_local->obecna_liczba_pszczol += otrzymana_ilosc_jaj;
                stan_ula_local->obecna_liczba_pszczol_ul += otrzymana_ilosc_jaj;
                stan_ula_dzielony->obecna_liczba_pszczol    = stan_ula_local->obecna_liczba_pszczol;
                stan_ula_dzielony->obecna_liczba_pszczol_ul = stan_ula_local->obecna_liczba_pszczol_ul;

            if (semop(sem_id, &unlock, 1) == -1) {
                    perror("[UL] semop unlock (nowe robotnice)");
                
                }
            printf("Laczenie z jajkami jest: %d osobikow\n", stan_ula_local->obecna_liczba_pszczol);
            for (int i = 0; i < otrzymana_ilosc_jaj; i++) {
                Pszczola* p = (Pszczola*) malloc(sizeof(Pszczola));
                if (!p) {
                    fprintf(stderr, "[UL] Błąd alokacji pszczoły.\n");
                    continue;
                }
                p->licznik_odwiedzen   = 0;
                p->liczba_cykli        = 2;
                p->pszczola_jest_w_ulu = 1; // od razu w ulu?

                Argumenty_Watku* args = (Argumenty_Watku*) malloc(sizeof(Argumenty_Watku));
                if (!args) {
                    fprintf(stderr, "[UL] Błąd alokacji Argumenty_Watku.\n");
                    free(p);
                    continue;
                }
                args->pszczola                  = p;
                args->sem_id                    = sem_id;
                args->stan_ula_do_przekazania   = stan_ula_dzielony;

                pthread_create(&nowe_robotnice[i], NULL, robotnica, (void*)args);
            }
            //pthread_mutex_lock(&wejscie_mutex);
            //wejscie_otwarte = 1;
           // pthread_cond_broadcast(&cond_dostepne_miejsce);
           // pthread_mutex_unlock(&wejscie_mutex);

            free(nowe_robotnice);
        }
        else
        {
            if (semop(sem_id, &lock, 1) == -1) {
                    perror("[UL] semoop lock (Aktualizacja danych error)");
                }
                stan_ula_dzielony->obecna_liczba_pszczol    = stan_ula_local->obecna_liczba_pszczol;
                stan_ula_dzielony->obecna_liczba_pszczol_ul = stan_ula_local->obecna_liczba_pszczol_ul;
                if (semop(sem_id, &unlock, 1) == -1) {
                    perror("[UL] semoop unlock (Aktualizacja danych error)");
                }
                printf("[UL] Stan: obecna=%d, w_ulu=%d, max=%d\n",
               stan_ula_local->obecna_liczba_pszczol,
               stan_ula_local->obecna_liczba_pszczol_ul,
               stan_ula_local->maksymalna_ilosc_osobnikow);
        }
    }

    for (int i = 0; i < liczba_poczatek; i++) {
        pthread_join(robotnice_tab[i], NULL);
    }
    free(robotnice_tab);

    // sprzatenie
    if (shmdt(stan_ula_dzielony) == -1) {
        perror("[UL] shmdt");
    }
    free(stan_ula_local);

    sem_destroy(&wejscie1);
    sem_destroy(&wejscie2);
    pthread_mutex_destroy(&liczba_pszczol_ul_mutex);
    pthread_cond_destroy(&cond_dostepne_miejsce);
    //pthread_cond_destroy(&cond_wejscie_otwarte);
    pthread_mutex_destroy(&wejscie_mutex);
    pthread_mutex_destroy(&blokada_ula);

    printf("[UL] Koncze proces ula.\n");
    return 0;
}
