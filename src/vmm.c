#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static int bitShift = 0;
static FILE* vmm_log;

unsigned int page_actives_index = 0;
bool allFrameUsed = false;
static unsigned int pages_actives[NUM_FRAMES];
static bool reference[NUM_FRAMES];

void vmm_init (FILE *log)
{
  // Initialise le fichier de journal.
  vmm_log = log;
  bitShift = (int) log2f(PAGE_FRAME_SIZE);
}


// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command (FILE *out, const char *command,
                             unsigned int laddress, /* Logical address. */
		             unsigned int page,
                             unsigned int offset,
                             unsigned int frame,
                             unsigned int paddress, /* Physical address.  */
		             char c) /* Caractère lu ou écrit.  */
{
  if (out)
    fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, laddress,
	     page, offset, frame, paddress);
}

unsigned int get_frame(unsigned int page, bool write) {
    unsigned int frame;
    // Regarder dans le TLB pour la page
    int frameSaved = tlb_lookup(page, write);
    if (frameSaved >= 0) {
        frame = frameSaved;
    } else {

        //si on trouve pas on regarde dans le page table(PT)
        frameSaved = pt_lookup(page);
        if (frameSaved >= 0) {
            frame = frameSaved;
        } else {
            // si on trouve pas dans la page table on la download
            //et on la mets dans le pt et dans le tlb

            // Il reste des frames vide
            if (page_actives_index < NUM_FRAMES && !allFrameUsed) {
                pages_actives[page_actives_index] = page;
                reference[page_actives_index] = false;
                frame = page_actives_index;

                if (page_actives_index == NUM_FRAMES - 1) {
                    allFrameUsed = true;
                }

                page_actives_index = page_actives_index + 1;
            } else {
                // On remplace une frame
                // Algorithme Second Chance
                bool replacementFound = false;
                while(!replacementFound) {
                    if (page_actives_index >= NUM_FRAMES) {
                        page_actives_index = 0;
                    }

                    if (reference[page_actives_index] == false) {
                        frame = page_actives_index;
                        replacementFound = true;
                    } else {
                        reference[page_actives_index] = false;
                    }
                    page_actives_index = page_actives_index + 1;
                }

                // Si la page est modifier, on la sauvegarde dans le BACKING_STORE
                if (!pt_readonly_p(pages_actives[frame])) {
                    pm_backup_page(frame, pages_actives[frame]);
                }
                pt_unset_entry(pages_actives[frame]);
            }

            pages_actives[frame] = page;

            pm_download_page(page, frame);
            pt_set_entry(page, frame);
            pt_set_readonly(page, !write);
        }
        tlb_add_entry(page, frame, !write);
    }
    reference[frame] = true;
    return frame;
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
    read_count++;
    // bit shift a droite de 8 pour avoir la page
    unsigned int page = laddress >> bitShift;
    // la difference pour le offset
    unsigned int offset = laddress - (page << bitShift);

    unsigned int frame = get_frame(page, false);

    // remettre dans paddress le  frame et offset ensemble en binaire.
    unsigned int paddress = (frame << bitShift)+ offset;

    // va lire le char
    char c = pm_read(paddress);

    // aujoute au log
    vmm_log_command (stdout, "READING", laddress, page, offset, frame, paddress, c);

    return c;
}


/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
    write_count++;
    // bit shift a droite de 8 pour avoir la page
    unsigned int page = laddress >> bitShift;
    // la difference pour le offset
    unsigned int offset = laddress - (page << bitShift);

    unsigned int frame = get_frame(page, true);

    // remettre dans paddress le  frame et offset ensemble en binaire.
    unsigned int paddress = (frame << bitShift) + offset;

    // va ecrire le char
    pm_write(paddress, c);
    pt_set_readonly(page, false);

    // aujoute au log
    vmm_log_command (stdout, "WRITING", laddress, page, offset, frame, paddress, c);
}


void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
