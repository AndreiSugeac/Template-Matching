#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct
{
    unsigned char B,G,R;
} pixel;

typedef struct
{
    int x,y;
    float corr;
    unsigned char *cul;
} detectie;

int padding(unsigned int W)
{
    int pad;
    if(W % 4 != 0)
        pad = (4 - (W * 3) % 4) % 4;
    else pad = 0;
    return pad;
}

void calc_W_H(unsigned char *nume,unsigned int *W,unsigned int *H,unsigned int  *dim)
{
    FILE *fin;
    fin = fopen(nume,"rb");
    if(fin == NULL)
    {
        printf("Eroare la deschidere!!!");
        return;
    }

    fseek(fin,2,SEEK_SET);
    fread(dim,sizeof(unsigned int),1,fin);
    fseek(fin,18,SEEK_SET);
    fread(W,sizeof(unsigned int),1,fin);
    fread(H,sizeof(unsigned int),1,fin);
    fseek(fin,0,SEEK_SET);
    fclose(fin);
}

void create_grayscale(unsigned char *sursa,unsigned char *destinatie)
{
    unsigned int W,H,dim;
    calc_W_H(sursa,&W,&H,&dim);
    FILE *fin,*fout;
    fin = fopen(sursa,"rb");
    fout = fopen(destinatie,"wb");
    if(fin == NULL)
    {
        printf("Nu s-a putut deschide fisierul care contine imaginea sursa!");
        return;
    }
    int pad;
    pad = padding(W);
    fseek(fin,0,SEEK_SET);
    fseek(fout,0,SEEK_SET);
    unsigned char c;
    int i;
    for(i = 0; i < 54; ++i)
    {
        fread(&c,sizeof(unsigned char),1,fin);
        fwrite(&c,sizeof(unsigned char),1,fout);
    }
    fflush(fout);
    fseek(fout,54,SEEK_SET);
    fseek(fin,54,SEEK_SET);
    int poz = 0;
    pixel P;
    while(1)
    {
        fread(&P.B,sizeof(unsigned char),1,fin);
        fread(&P.G,sizeof(unsigned char),1,fin);
        fread(&P.R,sizeof(unsigned char),1,fin);
        if(feof(fin)) break;
        unsigned char aux;
        aux = (0.299 * P.R + 0.587 * P.G + 0.114 * P.B);
        fwrite(&aux,sizeof(unsigned char),1,fout);
        fwrite(&aux,sizeof(unsigned char),1,fout);
        fwrite(&aux,sizeof(unsigned char),1,fout);
        poz++;
        if(poz == W)
        {
            int padd = pad;
            while(padd)
            {
                unsigned char p = 0;
                fwrite(&p,1,1,fout);
                padd--;
            }
            fseek(fin,pad,SEEK_CUR);
            poz = 0;
        }
    }
    fclose(fin);
    fclose(fout);
}

pixel **matricizare(unsigned char *sursa)
{
    unsigned int W,H,dim;
    calc_W_H(sursa,&W,&H,&dim);
    FILE *fin;
    fin = fopen(sursa,"rb");
    if(fin == NULL)
    {
        printf("Eroare la deschiderea fisierului care contine imaginea!!!");
        return;
    }
    int pad = padding(W);
    fseek(fin,54,SEEK_SET);
    pixel **mat;
    mat = (pixel**)calloc(H,sizeof(pixel*));
    int i;
    for(i = 0; i < H; ++i)
    {
        mat[i] = (pixel*)calloc(W,sizeof(pixel));
    }
    int j;
    for(i = 0; i < H; ++i)
    {
        for(j = 0; j < W; ++j)
        {
            fread(&(mat[i][j].B),sizeof(unsigned char),1,fin);
            fread(&(mat[i][j].G),sizeof(unsigned char),1,fin);
            fread(&(mat[i][j].R),sizeof(unsigned char),1,fin);
        }
        fseek(fin,pad,SEEK_CUR);
    }
    fclose(fin);
    return mat;
}

float med_grayscale(pixel **mat,unsigned int W,unsigned int H,int offset_i,int offset_j)
{
    float med = 0;
    int i,j;
    for(i = 0; i < H; ++i)
    {
        for(j = 0; j < W; ++j)
            med += mat[i + offset_i][j + offset_j].R;
    }
    med = (float)(med * (1.0 / (W * H)));
    return med;
}

float standard_deriv(pixel **mat,unsigned int i,unsigned int j,unsigned int offset_i,unsigned int offset_j)
{
    int l,c,n;
    n = i * j;
    float dev = 0;
    float avg = med_grayscale(mat,i,j,offset_i,offset_j);
    for(l = offset_i; l < i + offset_i; ++l)
    {
        for(c = offset_j; c < j + offset_j; ++c)
            dev += (float)(((float)mat[l][c].R - avg) * ((float)mat[l][c].R - avg));
    }
    dev = (float)(dev * (1.0 / (n - 1)));
    dev = (float)sqrt(dev);
    return dev;
}

float correlation(pixel **mat_i,pixel **mat_s,unsigned int W,unsigned int H,int offset_i,int offset_j)
{
    float corr = 0;
    int n = W * H;
    float dev_s = standard_deriv(mat_s,H,W,0,0);
    float dev_f = standard_deriv(mat_i,H,W,offset_i,offset_j);
    float sbar = med_grayscale(mat_i,W,H,0,0);
    int i,j;
    for(i = offset_i; i < H + offset_i; ++i)
    {
        for(j = offset_j; j < W + offset_j; ++j)
        {
            float fbar;
            fbar = med_grayscale(mat_i,W,H,offset_i,offset_j);
            corr = corr + ((1.0 /(dev_f * dev_s)) * ((mat_i[i][j].R - fbar) * (mat_s[i - offset_i][j - offset_j].R - sbar)));
        }
    }
    corr = (corr * (1.0 / n));
    return corr;

}

void select_color(unsigned char *sab,pixel *C)
{
    if(sab == "cifra0.bmp")
    {
        C->R = 255;
        C->G = 0;
        C->B = 0;
        return;
    }
    if(sab == "cifra1.bmp")
    {
        C->R = 255;
        C->G = 255;
        C->B = 0;
        return;
    }
    if(sab == "cifra2.bmp")
    {
        C->R = 0;
        C->G = 255;
        C->B = 0;
        return;
    }
    if(sab == "cifra3.bmp")
    {
        C->R = 0;
        C->G = 255;
        C->B = 255;
        return;
    }
    if(sab == "cifra4.bmp")
    {
        C->R = 255;
        C->G = 0;
        C->B = 255;
        return;
    }
    if(sab == "cifra5.bmp")
    {
        C->R = 0;
        C->G = 0;
        C->B = 255;
        return;
    }
    if(sab == "cifra6.bmp")
    {
        C->R = 192;
        C->G = 192;
        C->B = 192;
        return;
    }
    if(sab == "cifra7.bmp")
    {
        C->R = 255;
        C->G = 140;
        C->B = 0;
        return;
    }
    if(sab == "cifra8.bmp")
    {
        C->R = 128;
        C->G = 0;
        C->B = 128;
        return;
    }
    if(sab == "cifra9.bmp")
    {
        C->R = 128;
        C->G = 0;
        C->B = 0;
        return;
    }
}

void add_color(unsigned char *imag,pixel C,int x,int y,unsigned int W,unsigned int H,unsigned int bW,unsigned int bH)
{
    int pad = padding(bW);
    FILE *fout;
    fout = fopen(imag,"r+b");
    if(fout == NULL)
    {
        printf("Eroare la deschiderea fisierului care contine imaginea!!!");
        return;
    }
    fseek(fout,54 + (x * bW + y) * 3,SEEK_SET);
    int i,j;
    for(i = 0; i < W; ++i)
    {
        fwrite(&C.B,sizeof(unsigned char),1,fout);
        fwrite(&C.G,sizeof(unsigned char),1,fout);
        fwrite(&C.R,sizeof(unsigned char),1,fout);
        fflush(fout);
    }
    for(j = 0; j < H - 2; ++j)
    {
        fseek(fout,(bW - W - 1) * 3 + pad,SEEK_CUR);
        fwrite(&C.B,sizeof(unsigned char),1,fout);
        fwrite(&C.G,sizeof(unsigned char),1,fout);
        fwrite(&C.R,sizeof(unsigned char),1,fout);
        fflush(fout);

        fseek(fout,(W - 1) * 3,SEEK_CUR);
        fwrite(&C.B,sizeof(unsigned char),1,fout);
        fwrite(&C.G,sizeof(unsigned char),1,fout);
        fwrite(&C.R,sizeof(unsigned char),1,fout);
        fflush(fout);
    }
    fseek(fout,(bW - W - 1) * 3 + pad,SEEK_CUR);
    for(i = 0; i < W; ++i)
    {
        fwrite(&C.B,sizeof(unsigned char),1,fout);
        fwrite(&C.G,sizeof(unsigned char),1,fout);
        fwrite(&C.R,sizeof(unsigned char),1,fout);
        fflush(fout);
    }

    fclose(fout);
}

//functia principala a algoritmului de template matching
void template_matching(unsigned char *imag,unsigned char *sablon,float ps,detectie **D,int *poz)
{
    unsigned char *img_gray = "imag_gray.bmp",*sab_gray = "sab_gray.bmp";
    //creem doua imagini grayscale din imaginea originala si sablonul trimise ca parametrii
    create_grayscale(imag,img_gray);
    create_grayscale(sablon,sab_gray);
    unsigned int bmpW,bmpH,bmpD;
    //calculam dimensiunile pentru imaginea originala
    calc_W_H(img_gray,&bmpW,&bmpH,&bmpD);
    unsigned int W,H,dim;
    //calculam dimensiunile pentru un sablon
    calc_W_H(sab_gray,&W,&H,&dim);
    pixel **mat_i,**mat_s;
    //salvam pixelii din imaginea originala intr-o matrice
    mat_i = matricizare(img_gray);
    //salvam pixelii din sablon intr-o matrice
    mat_s = matricizare(sab_gray);
    int x,y,offset_i = 0,offset_j = 0;
    for(x = 0; x < bmpH - H; ++x)
    {
        for(y = 0; y < bmpW - W; ++y)
        {
            float corr;
            //calculam corelatia pentru fiecare pozitie din matrice
            corr = correlation(mat_i,mat_s,W,H,x,y);
            if(corr >= ps)
            {
                //salvam detectia in vectorul de detectii in cazul in care corelatia e mai mare decat pragul ps stabilit
                (*((*D) + (*poz))).corr = corr;
                (*((*D) + (*poz))).x = x;
                (*((*D) + (*poz))).y = y;
                (*((*D) + (*poz))).cul= sablon;
                (*poz)++;
            }
        }
    }
}

int cmp(const void *a,const void *b)
{
    detectie da = *(detectie*)a;
    detectie db = *(detectie*)b;
    if(da.corr < db.corr) return 1;
    else return -1;
}

float suprapunere(detectie A,detectie B,unsigned int W,unsigned int H)
{
    float sup = 0;
    float inter;
    float reun = 0;

    if(abs((A.x + H - 1) - (B.x + H - 1)) <= H - 1 && abs((A.y + W - 1) - (B.y + W - 1)) <= W - 1)
        inter = abs((A.x + H - 1) - (B.x + H - 1)) * abs((A.y + W - 1) - (B.y + W - 1));
    else return 0;
    reun = (A.x + H - 1) * (A.y + W - 1) + (B.x + H - 1) * (B.y + W - 1) - inter;

    sup = (reun) * (1.0 / inter);
    return sup;
}

void elim_non_maxime(unsigned char *imag,detectie *D,int poz,unsigned int bW,unsigned int bH,unsigned int W,unsigned int H)
{
    int i,j,afis;
    for(i = poz - 1; i > 0; --i)
    {
        afis = i;
        for(j = i; j >= 0; --j)
            if(suprapunere(D[j],D[i],W,H) > 0.2)
            {
                afis = j;
            }
        pixel C;
        select_color(D[afis].cul,&C);
        add_color(imag,C,D[afis].x,D[afis].y,W,H,bW,bH);
    }
}

int main()
{
    unsigned char *imag = "test.bmp";
    unsigned char *sablon0 = "cifra0.bmp";
    unsigned char *sablon1 = "cifra1.bmp";
    unsigned char *sablon2 = "cifra2.bmp";
    unsigned char *sablon3 = "cifra3.bmp";
    unsigned char *sablon4 = "cifra4.bmp";
    unsigned char *sablon5 = "cifra5.bmp";
    unsigned char *sablon6 = "cifra6.bmp";
    unsigned char *sablon7 = "cifra7.bmp";
    unsigned char *sablon8 = "cifra8.bmp";
    unsigned char *sablon9 = "cifra9.bmp";

    detectie *D;
    D = (detectie*)calloc(10000,sizeof(detectie));
    int poz = 0;

    template_matching(imag,sablon0,0.50,&D,&poz);
    template_matching(imag,sablon1,0.50,&D,&poz);
    template_matching(imag,sablon2,0.50,&D,&poz);
    template_matching(imag,sablon3,0.50,&D,&poz);
    template_matching(imag,sablon4,0.50,&D,&poz);
    template_matching(imag,sablon5,0.50,&D,&poz);
    template_matching(imag,sablon6,0.50,&D,&poz);
    template_matching(imag,sablon7,0.50,&D,&poz);
    template_matching(imag,sablon8,0.50,&D,&poz);
    template_matching(imag,sablon9,0.50,&D,&poz);

    qsort(D,poz,sizeof(detectie),cmp);

    unsigned int bmpW,bmpH,bmpD;
    calc_W_H(imag,&bmpW,&bmpH,&bmpD);

    unsigned int W,H,dim;
    calc_W_H(sablon0,&W,&H,&dim);

    //"template_matching.bmp" este o copie a imaginii originale "test.bmp", pe care vor fi afisate detectiile
    //este necesar sa existe in folderul programului
    unsigned char *temp = "template_matching.bmp";

    elim_non_maxime(temp,D,poz,bmpW,bmpH,W,H);
    return 0;
}
