
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "tlb.h"

#include "conf.h"

struct tlb_entry
{
  unsigned int page_number;
  int frame_number;             /* Invalide si négatif.  */
  bool readonly : 1;
};

static FILE *tlb_log = NULL;
static struct tlb_entry tlb_entries[TLB_NUM_ENTRIES]; 

static unsigned int tlb_hit_count = 0;
static unsigned int tlb_miss_count = 0;
static unsigned int tlb_mod_count = 0;
static int ordre[8] = {0,1,2,3,4,5,6,7};

/* Initialise le TLB, et indique où envoyer le log des accès.  */
void tlb_init (FILE *log)
{
  for (int i = 0; i < TLB_NUM_ENTRIES; i++)
    tlb_entries[i].frame_number = -1;
  tlb_log = log;
}

/******************** ¡ NE RIEN CHANGER CI-DESSUS !  ******************/

/* Recherche dans le TLB.
 * Renvoie le `frame_number`, si trouvé, ou un nombre négatif sinon.  */
static int tlb__lookup (unsigned int page_number, bool write)
{
    int resultat = -1;
  for(int index = 0; index < TLB_NUM_ENTRIES; index++){
      if(page_number == tlb_entries[index].page_number){
          if (write) {
              tlb_entries[index].readonly = false;
          }
          resultat = tlb_entries[index].frame_number;

          // met l'index a la fin de la liste
          for (int j = 0; j < TLB_NUM_ENTRIES - 1; j++){
              if (ordre[j] == index){
                  int temp = ordre[j];
                  ordre[j] = ordre[j+1];
                  ordre[j+1] = temp;
              }
          }
      }
  }
  return resultat;
}

/* Ajoute dans le TLB une entrée qui associe `frame_number` à
 * `page_number`.  */
static void tlb__add_entry (unsigned int page_number,
                            unsigned int frame_number, bool readonly)
{
    // choisi l'index le moins utiliser
    int choix = ordre[0];
    tlb_entries[choix].frame_number = frame_number;
    tlb_entries[choix].page_number = page_number;
    tlb_entries[choix].readonly = readonly;
    // met l'index a la fin de la liste
    for (int j = 0; j < TLB_NUM_ENTRIES - 1; j++) {
        if (ordre[j] == choix) {
            int temp = ordre[j];
            ordre[j] = ordre[j + 1];
            ordre[j + 1] = temp;
        }
    }

}

/******************** ¡ NE RIEN CHANGER CI-DESSOUS !  ******************/

void tlb_add_entry (unsigned int page_number,
                    unsigned int frame_number, bool readonly)
{
  tlb_mod_count++;
  tlb__add_entry (page_number, frame_number, readonly);
}

int tlb_lookup (unsigned int page_number, bool write)
{
  int fn = tlb__lookup (page_number, write);
  (*(fn < 0 ? &tlb_miss_count : &tlb_hit_count))++;
  return fn;
}

/* Imprime un sommaires des accès.  */
void tlb_clean (void)
{
  fprintf (stdout, "TLB misses   : %3u\n", tlb_miss_count);
  fprintf (stdout, "TLB hits     : %3u\n", tlb_hit_count);
  fprintf (stdout, "TLB changes  : %3u\n", tlb_mod_count);
  fprintf (stdout, "TLB miss rate: %.1f%%\n",
           100 * tlb_miss_count
           /* Ajoute 0.01 pour éviter la division par 0.  */
           / (0.01 + tlb_hit_count + tlb_miss_count));
}
