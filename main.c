/*
 * gminibat:
 * 
 * This useful tool reads the battery capacity from the linux sysfs
 * and displays a 24x24 status icon.
 * 
 * v0.1: Works basically, TODO: check return codes
 * 
 * 
 * This program is licensed under the GPLv3
 * 
 * 
 * Author: Sebastian Waniorek (sebastia@wainiorek.de)
 */

#include <gtk-2.0/gtk/gtk.h>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>

#define PIX_W 24
#define PIX_H 24

#define FILL_LO 19
#define FILL_HI  6

static int lastfullcap;
static int currentcap;    
static char powerstate[NAME_MAX];
static char syspath[PATH_MAX] = "/sys/class/power_supply/";
static char capname[PATH_MAX];
static GdkPixbuf *pixbuf;
static GtkStatusIcon *ico;

static const guchar CLEAR[]  = {255, 255, 255,   0};
static const guchar BAT_BG[] = {255, 255, 255, 255};
static const guchar BAT_LN[] = {  0,   0,   0, 255};
static const guchar BAT_OK[] = {  0, 255,   0, 255};
static const guchar BAT_MD[] = {255, 165,   0, 255};
static const guchar BAT_EP[] = {255,   0,   0, 255};

static inline void fillRect(const guchar* c,
                            int x0, int y0, int x1, int y1) {
    int x, y;
    guchar *px = gdk_pixbuf_get_pixels(pixbuf);
    int stride = gdk_pixbuf_get_rowstride(pixbuf);
    for (x = x0; x <= x1; x++) {
        for (y = y0; y <= y1; y++) {
            memcpy(px+(y)*stride+(x)*4, c, 4);
        }
    }
}

static inline void fillPx(const guchar* c, int x, int y) {
    guchar *px = gdk_pixbuf_get_pixels(pixbuf);
    int stride = gdk_pixbuf_get_rowstride(pixbuf);
    memcpy(px+(y)*stride+(x)*4, c, 4);
}

static inline void drawFrame(void) {
    fillRect(BAT_LN, 10,  2,  13,  3);
    fillRect(BAT_LN,  5,  4,  18, 21); 
}
static inline void drawFlash(void) {
    fillRect(BAT_LN, 10, 11, 11, 12);
    fillRect(BAT_LN, 12, 12, 13, 13);
    fillRect(BAT_LN, 11, 10, 12, 10);
    fillRect(BAT_LN, 11, 14, 13, 14);
    fillPx(BAT_LN, 12,  9);
    fillPx(BAT_LN, 13,  8);
    fillPx(BAT_LN, 12, 15);
    fillPx(BAT_LN, 10, 16);
}

// remaining capacity from 0..1
static inline void drawFill(float remaining) {
    const guchar* c = BAT_OK;
    if (remaining < 0.5) c = BAT_MD;
    if (remaining < 0.2) c = BAT_EP;
    
    int top = ((float)FILL_LO+
               (float)(FILL_HI-FILL_LO)*remaining);    

    fillRect(BAT_BG,  8, FILL_HI, 15, top    );
    fillRect(c,       8, top,     15, FILL_LO);
}

static void findBattPath(void) {
    DIR *dp;
    struct dirent *ep;

    dp = opendir (syspath);
    if (dp != NULL)  {
    while (ep = readdir (dp)) {
        if (strstr(ep->d_name, "BAT"))
        break;
    }
    strcat(syspath, ep->d_name);
    strcat(syspath, "/");
    puts(syspath);
    }
    (void) closedir (dp);
 }

static void updateData(int sig) {
    sig=sig;
    
    strncpy(capname, syspath, PATH_MAX);
    strcat(capname, "energy_now");
    FILE* fp = fopen(capname, "r");
    if (fp == NULL) {
        strncpy(capname, syspath, PATH_MAX);
        strcat(capname, "charge_now");
        fp = fopen(capname, "r");
    }

    fscanf(fp, "%d", &currentcap);
    fclose(fp);

    strncpy(capname, syspath, PATH_MAX);
    strcat(capname, "status");
    fp = fopen(capname, "r");
    fscanf(fp, "%s", powerstate);
    fclose(fp);

    snprintf(capname, PATH_MAX, "%.1f %%",
             (float)currentcap/(float)lastfullcap*100.0);
    gtk_status_icon_set_tooltip(ico, capname);

    fillRect(CLEAR, 0, 0, 23, 23);
    drawFrame();
    drawFill((float)currentcap/(float)lastfullcap);
    if (strcmp(powerstate, "Charging")==0) drawFlash();
    
    gtk_status_icon_set_from_pixbuf(ico, pixbuf);
    alarm(2);
}

#if 0
void tray_icon_on_click(GtkStatusIcon *status_icon, 
            gpointer user_data) {
    printf("Clicked on tray icon\n");
}

void tray_icon_on_menu(GtkStatusIcon *status_icon, guint button, 
               guint activate_time, gpointer user_data) {
    printf("Popup menu\n");
}
#endif

static GtkStatusIcon *create_tray_icon() {
    GtkStatusIcon *tray_icon;

    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                            TRUE, 8, PIX_W, PIX_H);
    
    tray_icon = gtk_status_icon_new_from_pixbuf(pixbuf);

#if 0
    g_signal_connect(G_OBJECT(tray_icon), "activate", 
             G_CALLBACK(tray_icon_on_click), NULL);
    g_signal_connect(G_OBJECT(tray_icon), 
             "popup-menu",
             G_CALLBACK(tray_icon_on_menu), NULL);
#endif
 
    gtk_status_icon_set_visible(tray_icon, TRUE);

    return tray_icon;
}

int main(int argc, char **argv) {
    (void) signal(SIGALRM, updateData);
    findBattPath();

    strncpy(capname, syspath, PATH_MAX);
    strcat(capname, "energy_full");
    FILE* fp = fopen(capname, "r");
    if (fp == NULL) {
        strncpy(capname, syspath, PATH_MAX);
        strcat(capname, "charge_full");
        fp = fopen(capname, "r");
    }
    fscanf(fp, "%d", &lastfullcap);
    fclose(fp);
    printf("lastfull: %d\n", lastfullcap);

    gtk_init(&argc, &argv);
    ico = create_tray_icon();

    alarm(2);
    updateData(0);

    gtk_main();

    return 0;
}
