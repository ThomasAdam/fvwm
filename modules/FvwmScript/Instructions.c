/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "types.h"
#include "libs/fvwmsignal.h"

extern int fd[2];
extern Window ref;

void (*TabCom[25]) (int NbArg,long *TabArg);
char *(*TabFunc[22]) (int *NbArg, long *TabArg);
int (*TabComp[7]) (char *arg1,char *arg2);

extern Display *dpy;
extern int screen;
extern X11base *x11base;
extern int grab_serve;
extern struct XObj *tabxobj[1000];
extern void LoadIcon(struct XObj *xobj);

extern int nbobj;
extern char **TabVVar;
extern int TabIdObj[1001];
extern char *ScriptName;
extern char *ModuleName;
extern TypeBuffSend BuffSend;
extern Atom propriete;

char *BufCom;
char Command[255]="None";
time_t TimeCom=0;

/*************************************************************/
/* Ensemble de fonction de comparaison de deux entiers       */
/*************************************************************/
static int Inf(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (an1<an2);
}

static int InfEq(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (an1<=an2);
}

static int Equal(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (strcmp(arg1,arg2)==0);
}

static int SupEq(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (an1>=an2);
}

static int Sup(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (an1>an2);
}

static int Diff(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (strcmp(arg1,arg2)!=0);
}

/*****************************************************/
/* Fonction qui retourne la valeur d'un argument     */
/*****************************************************/
static char *CalcArg (long *TabArg,int *Ix)
{
 char *TmpStr;
 int i;

 if (TabArg[*Ix]>100000)	/* Cas du codage d'un nombre */
 {
  i=(int)TabArg[*Ix]-200000;
  TmpStr=(char*)calloc(1,sizeof(char)*10);
  sprintf(TmpStr,"%d",i);
 }
 else if (TabArg[*Ix]<-200000)/* Cas d'un id de fonction de comparaison */
 {
  i=TabArg[*Ix]+250000;
  TmpStr=(char*)calloc(1,sizeof(char)*10);
  sprintf(TmpStr,"%d",i);
 }
 else if (TabArg[*Ix]< -100000)	/* Cas d'un id de fonction */
 {
  TmpStr=TabFunc[TabArg[*Ix]+150000](Ix,TabArg);
 }
 else				/* Cas d'une variable */
 {
  TmpStr=strdup(TabVVar[TabArg[*Ix]]);
 }
 return (TmpStr);
}

/*************************************************************/
/* Ensemble des fonctions pour recuperer les prop d'un objet */
/*************************************************************/
static char *FuncGetValue(int *NbArg, long *TabArg)
{
 char *tmp;
 long Id;

 (*NbArg)++;			/* La fonction GetValue n'a qu'un seul argument */
 tmp=CalcArg(TabArg,NbArg);
 Id=atoi(tmp);
 free(tmp);
 tmp=(char*)calloc(1,sizeof(char)*10);
 sprintf(tmp,"%d",tabxobj[TabIdObj[Id]]->value);
 return tmp;
}

static char *FuncGetMinValue(int *NbArg, long *TabArg)
{
 char *tmp;
 long Id;

 (*NbArg)++;			/* La fonction GetValue n'a qu'un seul argument */
 tmp=CalcArg(TabArg,NbArg);
 Id=atoi(tmp);
 free(tmp);
 tmp=(char*)calloc(1,sizeof(char)*10);
 sprintf(tmp,"%d",tabxobj[TabIdObj[Id]]->value2);
 return tmp;
}

static char *FuncGetMaxValue(int *NbArg, long *TabArg)
{
 char *tmp;
 long Id;

 (*NbArg)++;			/* La fonction GetValue n'a qu'un seul argument */
 tmp=CalcArg(TabArg,NbArg);
 Id=atoi(tmp);
 free(tmp);
 tmp=(char*)calloc(1,sizeof(char)*10);
 sprintf(tmp,"%d",tabxobj[TabIdObj[Id]]->value3);
 return tmp;
}


static char *FuncGetFore(int *NbArg, long *TabArg)
{
 char *tmp;
 long Id;
 XColor color;

 (*NbArg)++;			/* La fonction GetValue n'a qu'un seul argument */
 tmp=CalcArg(TabArg,NbArg);
 Id=atoi(tmp);
 free(tmp);
 tmp=(char*)calloc(1,sizeof(char)*7);
 color.pixel = tabxobj[TabIdObj[Id]]->TabColor[fore];
 XQueryColor(dpy, Pcmap, &color);
 sprintf(tmp, "%02x%02x%02x", color.red >> 8, color.green >> 8, color.blue >> 8);
 return tmp;
}

static char *FuncGetBack(int *NbArg, long *TabArg)
{
 char *tmp;
 long Id;
 XColor color;

 (*NbArg)++;			/* La fonction GetValue n'a qu'un seul argument */
 tmp=CalcArg(TabArg,NbArg);
 Id=atoi(tmp);
 free(tmp);
 tmp=(char*)calloc(1,sizeof(char)*7);
 color.pixel = tabxobj[TabIdObj[Id]]->TabColor[back];
 XQueryColor(dpy, Pcmap, &color);
 sprintf(tmp, "%02x%02x%02x", color.red >> 8, color.green >> 8, color.blue >> 8);
 return tmp;
}

static char *FuncGetHili(int *NbArg, long *TabArg)
{
 char *tmp;
 long Id;
 XColor color;

 (*NbArg)++;			/* La fonction GetValue n'a qu'un seul argument */
 tmp=CalcArg(TabArg,NbArg);
 Id=atoi(tmp);
 free(tmp);
 tmp=(char*)calloc(1,sizeof(char)*7);
 color.pixel = tabxobj[TabIdObj[Id]]->TabColor[hili];
 XQueryColor(dpy, Pcmap, &color);
 sprintf(tmp, "%02x%02x%02x", color.red >> 8, color.green >> 8, color.blue >> 8);
 return tmp;
}

static char *FuncGetShad(int *NbArg, long *TabArg)
{
 char *tmp;
 long Id;
 XColor color;

 (*NbArg)++;			/* La fonction GetValue n'a qu'un seul argument */
 tmp=CalcArg(TabArg,NbArg);
 Id=atoi(tmp);
 free(tmp);
 tmp=(char*)calloc(1,sizeof(char)*7);
 color.pixel = tabxobj[TabIdObj[Id]]->TabColor[shad];
 XQueryColor(dpy, Pcmap, &color);
 sprintf(tmp, "%02x%02x%02x", color.red >> 8, color.green >> 8, color.blue >> 8);
 return tmp;
}

/* Fonction qui retourne le titre d'un objet */
static char *FuncGetTitle(int *NbArg, long *TabArg)
{
 char *tmp;
 long Id;

 (*NbArg)++;
 tmp=CalcArg(TabArg,NbArg);
 Id=atoi(tmp);
 free(tmp);
 if (TabIdObj[Id]!=-1)
  tmp=strdup(tabxobj[TabIdObj[Id]]->title);
 else
 {
  fprintf(stderr,"Widget %d doesn't exist\n",(int)Id);
  tmp=(char*)calloc(1,sizeof(char));
  tmp[0]='\0';
 }
 return tmp;
}

/* Fonction qui retourne la sortie d'une commande */
static char *FuncGetOutput(int *NbArg, long *TabArg)
{
 char *cmndbuf;
 char *str;
 int line,index,i=2,j=0,k,NewWord;
 FILE *f;
 int maxsize=32000;
 int size;

 (*NbArg)++;
 cmndbuf=CalcArg(TabArg,NbArg);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 line=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 index=atoi(str);
 free(str);

 if ((strcmp(Command,cmndbuf))||((time(NULL)-TimeCom)>1)||(TimeCom==0))
 {
  if ((f = popen(cmndbuf,"r")) == NULL)
  {
   fprintf(stderr,"%s: can't run %s\n",ScriptName,cmndbuf);
   str=(char*)calloc(sizeof(char),10);
   free(cmndbuf);
   return str;
  }
  else
  {
   if (strcmp(Command,"None"))
    free(BufCom);
   BufCom=(char*)calloc(sizeof(char),maxsize);
   size=fread(BufCom,1,maxsize,f);
   pclose(f);
   strncpy(Command,cmndbuf,256);
   TimeCom=time(NULL);
  }
 }

 /* Recherche de la ligne */
 while ((i<=line)&&(BufCom[j]!='\0'))
 {
  j++;
  if (BufCom[j]=='\n') i++;
 }

 /* Recherche du mot */
 if (index!=-1)
 {
  if (i!=2) j++;
  i=1;
  NewWord=0;
  while ((i<index)&&(BufCom[j]!='\n')&&(BufCom[j]!='\0'))
  {
   j++;
   if (BufCom[j]==' ')
   {
    if (NewWord)
    {
     i++;
     NewWord=0;
    }
   }
   else
    if (!NewWord) NewWord=1;
  }
  str=(char*)calloc(sizeof(char),255);
  sscanf(&BufCom[j],"%s",str);
 }
 else		/* Lecture de la ligne complete */
 {
  if (i!=2) j++;
  k=j;
  while ((BufCom[k]!='\n')&&(BufCom[k]!='\0'))
   k++;
  str=(char*)calloc(sizeof(char),k-j+1);
  memmove(str,&BufCom[j],k-j);
  str[k-j]='\0';
 }

 free(cmndbuf);
 return str;
}

/* Convertion decimal vers hexadecimal */
static char *FuncNumToHex(int *NbArg, long *TabArg)
{
 char *str;
 int value,nbchar;
 int i,j;

 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 value=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 nbchar=atoi(str);
 free(str);

 str=(char*)calloc(1,nbchar+10);
 sprintf(str,"%X",value);
 j=strlen(str);
 if (j<nbchar)
 {
  memmove(&str[nbchar-j],str,j);
  for (i=0;i<(nbchar-j);i++)
   str[i]='0';
 }

 return str;
}

/* Convertion hexadecimal vers decimal */
static char *FuncHexToNum(int *NbArg, long *TabArg)
{
 char *str,*str2;
 int k;

 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 if (str[0]=='#')
  memmove(str,&str[1],strlen(str));
 k=(int)strtol(str,NULL,16);
 free(str);

 str2=(char*)calloc(1,20);
 sprintf(str2,"%d",k);
 return str2;
}

static char *FuncAdd(int *NbArg, long *TabArg)
{
 char *str;
 int val1,val2;

 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val1=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val2=atoi(str);
 free(str);
 str=(char*)calloc(1,20);
 sprintf(str,"%d",val1+val2);
 return str;
}

static char *FuncMult(int *NbArg, long *TabArg)
{
 char *str;
 int val1,val2;

 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val1=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val2=atoi(str);
 free(str);
 str=(char*)calloc(1,20);
 sprintf(str,"%d",val1*val2);
 return str;
}

static char *FuncDiv(int *NbArg, long *TabArg)
{
 char *str;
 int val1,val2;

 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val1=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val2=atoi(str);
 free(str);
 str=(char*)calloc(1,20);
 sprintf(str,"%d",val1/val2);
 return str;
}

static char *RemainderOfDiv(int *NbArg, long *TabArg)
{
#ifndef HAVE_DIV
 return strdup("Unsupported function: div");
#else
 char *str;
 int val1,val2;
 div_t res;

 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val1=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val2=atoi(str);
 free(str);
 str=(char*)calloc(1,20);
 res=div(val1,val2);
 sprintf(str,"%d",res.rem);
 return str;
#endif
}


static char *FuncStrCopy(int *NbArg, long *TabArg)
{
 char *str,*strsrc;
 int i1,i2;

 (*NbArg)++;
 strsrc=CalcArg(TabArg,NbArg);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 i1=atoi(str);
 if (i1<1) i1=1;
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 i2=atoi(str);
 if (i2<1) i2=1;
 free(str);

 if ((i1<=i2)&&(i1<=strlen(strsrc)))
 {
  if (i2>strlen(strsrc)) i2=strlen(strsrc);
  str=(char*)calloc(1,i2-i1+2);
  memmove(str,&strsrc[i1-1],i2-i1+1);
 }
 else
 {
  str=(char*)calloc(1,1);
 }

 free(strsrc);
 return str;
}

/* Lancement d'un script avec pipe */
static char *LaunchScript (int *NbArg,long *TabArg)
{
 char *arg,*execstr,*str,*scriptarg,*scriptname;
 unsigned long leng;
 int i;
 Atom MyAtom;

 /* Lecture des arguments */
 (*NbArg)++;
 arg=CalcArg(TabArg,NbArg);

 str=(char*)calloc(100,sizeof(char));

 /* Calcul du nom du script fils */
 x11base->TabScriptId[x11base->NbChild+2]=(char*)calloc(strlen(x11base->TabScriptId[1])+4,sizeof(char));

 if (x11base->NbChild<98)
 {
  i=16;
  do
  {
   sprintf(x11base->TabScriptId[x11base->NbChild+2],"%s%x",x11base->TabScriptId[1],i);
   MyAtom=XInternAtom(dpy,x11base->TabScriptId[x11base->NbChild+2],False);
   i++;
  }
  while (XGetSelectionOwner(dpy,MyAtom)!=None);
 }
 else
 {
  fprintf(stderr,"Too many launched script\n");
  sprintf(str,"-1");
  return str;
 }

 /* Construction de la commande */
 execstr=(char*)calloc(strlen(ModuleName)+strlen(arg)+
	strlen(x11base->TabScriptId[x11base->NbChild+2])+5,sizeof(char));
 scriptname=(char*)calloc(sizeof(char),100);
 sscanf(arg,"%s",scriptname);
 scriptarg=(char*)calloc(sizeof(char),strlen(arg));
 scriptarg=(char*)strncpy(scriptarg,&arg[strlen(scriptname)],strlen(arg)-strlen(scriptname));
 sprintf(execstr,"%s %s %s %s",ModuleName,scriptname,
		x11base->TabScriptId[x11base->NbChild+2],scriptarg);
 free(scriptname);
 free(scriptarg);
 free(arg);

 /* Envoi de la commande */
 write(fd[0], &ref, sizeof(Window));
 leng = strlen(execstr);
 write(fd[0], &leng, sizeof(unsigned long));
 write(fd[0], execstr, leng);
 leng = 1;
 write(fd[0], &leng, sizeof(unsigned long));
 free(execstr);

 /* Retourne l'id du fils */
 sprintf(str,"%d",x11base->NbChild+2);
 x11base->NbChild++;
 return str;
}

static char *GetScriptFather (int *NbArg,long *TabArg)
{
 char *str;

 str=(char*)calloc(10,sizeof(char));
 sprintf(str,"0");
 return str;
}

static char *GetTime (int *NbArg,long *TabArg)
{
 char *str;
 time_t t;

 str=(char*)calloc(20,sizeof(char));
 t=time(NULL);
 sprintf(str,"%ld",(long)t-x11base->BeginTime);
 return str;
}

static char *GetScriptArg (int *NbArg,long *TabArg)
{
 char *str;
 int val1;

 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val1=atoi(str);
 free(str);

 if (x11base->TabArg[val1] != NULL)
 {
   str=(char*)calloc(strlen(x11base->TabArg[val1])+1,sizeof(char));
   str=strcpy(str,x11base->TabArg[val1]);
 }
 else
 {
   str=(char*)calloc(1,sizeof(char));
   str=strcpy(str,"");
 }

 return str;
}

static char *ReceivFromScript (int *NbArg,long *TabArg)
{
 char *arg,*msg;
 int send;
 Atom AReceiv,ASend,type;
 static XEvent event;
 unsigned long longueur,octets_restant;
 unsigned char *donnees=(unsigned char *)"";
 int format;
 int NbEssai=0;

 (*NbArg)++;
 arg=CalcArg(TabArg,NbArg);
 send=(int)atoi(arg);
 free(arg);

 msg=(char*)calloc(256,sizeof(char));
 sprintf(msg,"No message");

 /* Recuperation des atomes */
 AReceiv=XInternAtom(dpy,x11base->TabScriptId[1],True);
 if (AReceiv==None)
 {
  fprintf(stderr,"Error with atome\n");
  return msg;
 }

 if ((send>=0)&&(send<99))
 {
     if (x11base->TabScriptId[send]!=NULL)
     {
	 ASend=XInternAtom(dpy,x11base->TabScriptId[send],True);
	 if (ASend==None)
	     fprintf(stderr,"Error with atome\n");
     }
     else
	 return msg;
 }
 else
     return msg;

 /* Recuperation du message */
 XConvertSelection(dpy,ASend,AReceiv,propriete,x11base->win,CurrentTime);
 while ((!XCheckTypedEvent(dpy,SelectionNotify,&event))&&(NbEssai<10))
 {
   usleep(1);
  NbEssai++;
 }
 if (event.xselection.property!=None)
  if (event.xselection.selection==ASend)
  {
   XGetWindowProperty(dpy,event.xselection.requestor,event.xselection.property,
		      0,8192,False,event.xselection.target,&type,&format,
		      &longueur,&octets_restant,&donnees);
   if (longueur>0)
   {
    msg=(char*)realloc((void*)msg,(longueur+1)*sizeof(char));
    msg=strcpy(msg,(char *)donnees);
    XDeleteProperty(dpy,event.xselection.requestor,event.xselection.property);
    XFree(donnees);
   }
  }
 return msg;
}


/***********************************************/
/* Ensemble des commandes possible pour un obj */
/***********************************************/

static void Exec (int NbArg,long *TabArg)
{
 unsigned long leng = 0;
 char *execstr;
 char *tempstr;
 int i;

 for (i=0;i<NbArg;i++)
   leng += strlen(CalcArg(TabArg,&i));

 if (leng >= 998)
 {
   fprintf(stderr, "[FvwmScript]: too long command %i chars max 998\n",
	   (int)leng);
   return;
 }

 execstr=(char*)calloc(1,leng+1);
 for (i=0;i<NbArg;i++)
 {
  tempstr=CalcArg(TabArg,&i);
  execstr=strcat(execstr,tempstr);
  free(tempstr);
 }

 write(fd[0], &ref, sizeof(Window));
 leng = strlen(execstr);
 write(fd[0], &leng, sizeof(unsigned long));
 write(fd[0], execstr, leng);
 leng = 1;
 write(fd[0], &leng, sizeof(unsigned long));
 free(execstr);
}

static void HideObj (int NbArg,long *TabArg)
{
 char *arg[1];
 int IdItem;
 int i=0;

 arg[0]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];


 tabxobj[IdItem]->flags[0]=True;
 /* On cache la fentre pour la faire disparaitre */
 XUnmapWindow(dpy,tabxobj[IdItem]->win);
 free(arg[0]);
}

static void ShowObj (int NbArg,long *TabArg)
{
 char *arg[1];
 int IdItem;
 int i=0;

 arg[0]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];

 tabxobj[IdItem]->flags[0]=False;
 XMapWindow(dpy,tabxobj[IdItem]->win);
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);
 free(arg[0]);
}

static void ChangeValue (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);

 tabxobj[TabIdObj[atoi(arg[0])]]->value=atoi(arg[1]);
 /* On redessine l'objet pour le mettre a jour */
 if (tabxobj[TabIdObj[atoi(arg[0])]]->TypeWidget != SwallowExec)
   XClearWindow(dpy, tabxobj[TabIdObj[atoi(arg[0])]]->win);
 tabxobj[TabIdObj[atoi(arg[0])]]->DrawObj(tabxobj[TabIdObj[atoi(arg[0])]]);
 free(arg[0]);
 free(arg[1]);
}

static void ChangeValueMax (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int j;

 arg[0]=CalcArg(TabArg,&i);
 j=atoi(arg[0]);
 i++;
 arg[1]=CalcArg(TabArg,&i);

 tabxobj[TabIdObj[j]]->value3=atoi(arg[1]);
 /* On redessine l'objet pour le mettre a jour */
 if (tabxobj[TabIdObj[j]]->value>tabxobj[TabIdObj[j]]->value3)
 {
  tabxobj[TabIdObj[j]]->value=atoi(arg[1]);
  if (tabxobj[TabIdObj[j]]->TypeWidget != SwallowExec)
    XClearWindow(dpy, tabxobj[TabIdObj[j]]->win);
  tabxobj[TabIdObj[j]]->DrawObj(tabxobj[TabIdObj[j]]);
 }
 free(arg[0]);
 free(arg[1]);
}

static void ChangeValueMin (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int j;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 j=atoi(arg[0]);

 tabxobj[TabIdObj[j]]->value2=atoi(arg[1]);
 /* On redessine l'objet pour le mettre a jour */
 if (tabxobj[TabIdObj[j]]->value<tabxobj[TabIdObj[j]]->value2)
 {
  tabxobj[TabIdObj[j]]->value=atoi(arg[1]);
  if (tabxobj[TabIdObj[j]]->TypeWidget != SwallowExec)
    XClearWindow(dpy, tabxobj[TabIdObj[j]]->win);
  tabxobj[TabIdObj[j]]->DrawObj(tabxobj[TabIdObj[j]]);
 }
 free(arg[0]);
 free(arg[1]);
}

static void ChangePos (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[3];
 int an[3];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 i++;
 arg[2]=CalcArg(TabArg,&i);

 IdItem= TabIdObj[atoi(arg[0])];
 for (i=1;i<3;i++)
  an[i]=atoi(arg[i]);
 tabxobj[IdItem]->x=an[1];
 tabxobj[IdItem]->y=an[2];
 XMoveWindow(dpy,tabxobj[IdItem]->win,an[1],an[2]);

 free(arg[0]);
 free(arg[1]);
 free(arg[2]);

}

static void ChangeFont (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;
#ifdef I18N_MB
 char **ml;
 int mc;
 char *ds;
 XFontStruct **fs_list;
#else
 XFontStruct *xfont;
#endif

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];
 if (tabxobj[IdItem]->font)
   free(tabxobj[IdItem]->font);
 tabxobj[IdItem]->font=strdup(arg[1]);
#ifdef I18N_MB
 /* Hmm.. Fontset is not freed. However, original alogrithm does not consider
  * the situation of font-loading-falure.
  */
 if ((tabxobj[IdItem]->xfontset = XCreateFontSet(dpy, tabxobj[IdItem]->font, &ml, &mc, &ds)) == NULL)
  {
   fprintf(stderr,"Can't load fontset %s\n",tabxobj[IdItem]->font);
  }
 else
 {
  XFontsOfFontSet(tabxobj[IdItem]->xfontset, &fs_list, &ml);
  tabxobj[IdItem]->xfont = fs_list[0];
  XSetFont(dpy,tabxobj[IdItem]->gc,tabxobj[IdItem]->xfont->fid);
 }
#else
 if ((xfont=XLoadQueryFont(dpy,tabxobj[IdItem]->font))==NULL)
  {
   fprintf(stderr,"Can't load font %s\n",tabxobj[IdItem]->font);
  }
 else
 {
  XFreeFont(dpy,tabxobj[IdItem]->xfont);
  tabxobj[IdItem]->xfont=xfont;
  XSetFont(dpy,tabxobj[IdItem]->gc,tabxobj[IdItem]->xfont->fid);
 }
#endif
 if (tabxobj[IdItem]->TypeWidget != SwallowExec)
   XClearWindow(dpy, tabxobj[IdItem]->win);
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);
 free(arg[0]);
 free(arg[1]);
}

static void ChangeSize (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[3];
 int an[3];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 i++;
 arg[2]=CalcArg(TabArg,&i);

 IdItem= TabIdObj[atoi(arg[0])];
 for (i=1;i<3;i++)
  an[i]=atoi(arg[i]);
 tabxobj[IdItem]->width=an[1];
 tabxobj[IdItem]->height=an[2];
 XResizeWindow(dpy,tabxobj[IdItem]->win,an[1],an[2]);
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);
 free(arg[0]);
 free(arg[1]);
 free(arg[2]);
}

static void ChangeTitle (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];

 if (tabxobj[IdItem]->title)
   free(tabxobj[IdItem]->title);
 tabxobj[IdItem]->title=strdup(arg[1]);
 if (tabxobj[IdItem]->TypeWidget != SwallowExec)
   XClearWindow(dpy, tabxobj[IdItem]->win);
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);
 free(arg[0]);
 free(arg[1]);
}

static void ChangeIcon (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];
/* if (tabxobj[IdItem]->icon!=NULL)
 {
  free(tabxobj[IdItem]->icon);
  if (tabxobj[IdItem]->iconPixmap!=None)
   XFreePixmap(dpy,tabxobj[IdItem]->iconPixmap);
  if (tabxobj[IdItem]->icon_maskPixmap!=None)
   XFreePixmap(dpy,tabxobj[IdItem]->icon_maskPixmap);
 }*/
 tabxobj[IdItem]->icon=strdup(arg[1]);
 LoadIcon(tabxobj[IdItem]);
 if (tabxobj[IdItem]->TypeWidget != SwallowExec)
   XClearWindow(dpy, tabxobj[IdItem]->win);
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);
 free(arg[0]);
 free(arg[1]);
}

static void ChangeForeColor (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];

 /* Liberation de la couleur */
 if (tabxobj[IdItem]->colorset < 0)
   XFreeColors(dpy,Pcmap,(void*)(&(tabxobj[IdItem])->TabColor[fore]),1,0);

 tabxobj[IdItem]->forecolor=(char*)calloc(100,sizeof(char));
 sprintf(tabxobj[IdItem]->forecolor,"%s",arg[1]);

 tabxobj[IdItem]->TabColor[fore] = GetColor(tabxobj[IdItem]->forecolor);
 if (tabxobj[IdItem]->colorset >= 0) {
   tabxobj[IdItem]->TabColor[back] = GetColor(tabxobj[IdItem]->backcolor);
   tabxobj[IdItem]->TabColor[hili] = GetColor(tabxobj[IdItem]->hilicolor);
   tabxobj[IdItem]->TabColor[shad] = GetColor(tabxobj[IdItem]->shadcolor);
 }
 tabxobj[IdItem]->colorset = -1;

 if (tabxobj[IdItem]->TypeWidget != SwallowExec)
   XClearWindow(dpy, tabxobj[IdItem]->win);
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);

 free(arg[0]);
 free(arg[1]);

}

static void ChangeBackColor (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];

 /* Liberation de la couleur */
 if (tabxobj[IdItem]->colorset < 0)
   XFreeColors(dpy,Pcmap,(void*)(&(tabxobj[IdItem])->TabColor[back]),1,0);

 tabxobj[IdItem]->backcolor=(char*)calloc(100,sizeof(char));
 sprintf(tabxobj[IdItem]->backcolor,"%s",arg[1]);

 tabxobj[IdItem]->TabColor[back] = GetColor(tabxobj[IdItem]->backcolor);
 if (tabxobj[IdItem]->colorset >= 0) {
   tabxobj[IdItem]->TabColor[fore] = GetColor(tabxobj[IdItem]->forecolor);
   tabxobj[IdItem]->TabColor[hili] = GetColor(tabxobj[IdItem]->hilicolor);
   tabxobj[IdItem]->TabColor[shad] = GetColor(tabxobj[IdItem]->shadcolor);
 }
 tabxobj[IdItem]->colorset = -1;

 if (tabxobj[IdItem]->TypeWidget != SwallowExec) {
   XSetWindowBackground(dpy,
			tabxobj[IdItem]->win,
			tabxobj[IdItem]->TabColor[back]);
   XClearWindow(dpy, tabxobj[IdItem]->win);
 }
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);

 free(arg[0]);
 free(arg[1]);
}

static void ChangeMainColorset (int i)
{
 /* Liberation de la couleur */
 if (x11base->colorset < 0) {
   XFreeColors(dpy,Pcmap,&x11base->TabColor[fore],1,0);
   XFreeColors(dpy,Pcmap,&x11base->TabColor[back],1,0);
   XFreeColors(dpy,Pcmap,&x11base->TabColor[hili],1,0);
   XFreeColors(dpy,Pcmap,&x11base->TabColor[shad],1,0);
 }
 x11base->colorset = i;
 AllocColorset(i);
 x11base->TabColor[fore] = Colorset[i].fg;
 x11base->TabColor[back] = Colorset[i].bg;
 x11base->TabColor[hili] = Colorset[i].hilite;
 x11base->TabColor[shad] = Colorset[i].shadow;
 SetWindowBackground(dpy, x11base->win,x11base->size.width,x11base->size.height,
		     &Colorset[i], Pdepth, x11base->gc, True);
}

static void ChangeColorset (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 if (atoi(arg[0]) == 0) {
   ChangeMainColorset(atoi(arg[1]));
   return;
 }
 IdItem= TabIdObj[atoi(arg[0])];

 /* Liberation de la couleur */
 if (tabxobj[IdItem]->colorset < 0) {
   XFreeColors(dpy,Pcmap,&tabxobj[IdItem]->TabColor[fore],1,0);
   XFreeColors(dpy,Pcmap,&tabxobj[IdItem]->TabColor[back],1,0);
   XFreeColors(dpy,Pcmap,&tabxobj[IdItem]->TabColor[hili],1,0);
   XFreeColors(dpy,Pcmap,&tabxobj[IdItem]->TabColor[shad],1,0);
 }
 sscanf(arg[1], "%d", &i);
 tabxobj[IdItem]->colorset = i;
 AllocColorset(i);
 tabxobj[IdItem]->TabColor[fore] = Colorset[i].fg;
 tabxobj[IdItem]->TabColor[back] = Colorset[i].bg;
 tabxobj[IdItem]->TabColor[hili] = Colorset[i].hilite;
 tabxobj[IdItem]->TabColor[shad] = Colorset[i].shadow;

 if (tabxobj[IdItem]->TypeWidget != SwallowExec) {
   SetWindowBackground(dpy, tabxobj[IdItem]->win, tabxobj[IdItem]->width,
		       tabxobj[IdItem]->height, &Colorset[i], Pdepth,
		       tabxobj[IdItem]->gc, False);
   XClearWindow(dpy, tabxobj[IdItem]->win);
 }
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);

 free(arg[0]);
 free(arg[1]);
}

static void SetVar (int NbArg,long *TabArg)
{
 int i;
 char *str,*tempstr;

 str=(char*)calloc(sizeof(char),1);
 for (i=1;i<NbArg;i++)
 {
  tempstr=CalcArg(TabArg,&i);
  str=(char*)realloc((void*)str,sizeof(char)*(1+strlen(str)+strlen(tempstr)));
  str=strcat(str,tempstr);
  free(tempstr);
 }

 free(TabVVar[TabArg[0]]);
 TabVVar[TabArg[0]]=str;
}

static void SendSign (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;
 int TypeMsg;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];
 TypeMsg=atoi(arg[1]);
 SendMsg(tabxobj[IdItem],TypeMsg);
 free(arg[0]);
 free(arg[1]);
}

static void WarpPointer(int NbArg,long *TabArg)
{
 int i=0;
 char *arg;
 int IdItem;

 arg=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg)];
 /* Deplacement du pointeur sur l'objet */
 XWarpPointer(dpy,None,tabxobj[IdItem]->win,0,0,0,0,
	tabxobj[IdItem]->width/2,tabxobj[IdItem]->height+10);
 free(arg);
}

void Quit (int NbArg,long *TabArg)
{
 exit(0);
}

static void IfThen (int NbArg,long *TabArg)
{
 char *arg[10];
 int i,j;
 int CurrArg=0;
 int IdFuncComp = 0;

 /* Verification de la condition */
 for (j=0;j<NbArg-2;j++)
 {
  if (TabArg[j]>100000)	/* Cas du codage d'un nombre */
  {
   i=(int)TabArg[j]-200000;
   arg[CurrArg]=(char*)calloc(1,sizeof(char)*10);
   sprintf(arg[CurrArg],"%d",i);
   CurrArg++;
  }
  else if (TabArg[j]<-200000)/* Cas d'un id de fonction de comparaison */
  {
   IdFuncComp=TabArg[j]+250000;
  }
  else if (TabArg[j]<-100000)	/* Cas d'un id de fonction */
  {
   arg[CurrArg]=TabFunc[TabArg[j]+150000](&j,TabArg);
   CurrArg++;
  }
  else				/* Cas d'une variable */
  {
    arg[CurrArg]=strdup(TabVVar[TabArg[j]]);
    CurrArg++;
  }
 }

 /* Comparaison des arguments */
 if (TabComp[IdFuncComp](arg[0],arg[1]))
  ExecBloc((Bloc*)TabArg[NbArg-2]);
 else if (TabArg[NbArg-1]!=0)
  ExecBloc((Bloc*)TabArg[NbArg-1]);

 free(arg[0]);
 free(arg[1]);
}

/* Instruction boucle */
static void Loop (int NbArg,long *TabArg)
{
 int IdVar;
 char *arg[2];
 int limit[2];
 int i;
 int CurrArg=0;

 /* le premier argument est une variable */
 IdVar=TabArg[0];
 /*On ajuste la taille de la var pour contenir un nombre */
 TabVVar[TabArg[0]]=(char*)realloc(TabVVar[TabArg[0]],sizeof(char)*10);
 /* Calcul des 2 autres arguments */
 for (i=1;i<NbArg-1;i++)
 {
  if (TabArg[i]>100000)	/* Cas du codage d'un nombre */
  {
   i=(int)TabArg[i]-200000;
   arg[CurrArg]=(char*)calloc(1,sizeof(char)*10);
   sprintf(arg[CurrArg],"%d",i);
  }
  else if (TabArg[i]<-100000)	/* Cas d'un id de fonction */
  {
   arg[CurrArg]=TabFunc[TabArg[i]+150000](&i,TabArg);
  }
  else				/* Cas d'une variable */
    arg[CurrArg]=strdup(TabVVar[TabArg[i]]);
  CurrArg++;
 }
 limit[0]=atoi(arg[0]);
 limit[1]=atoi(arg[1]);
 if (limit[0]<limit[1])
  for (i=limit[0];i<=limit[1];i++)
  {
   /* On met a jour la variable */
   sprintf(TabVVar[TabArg[0]],"%d",i);
   ExecBloc((Bloc*)TabArg[NbArg-1]);
  }
 else
  for (i=limit[0];i<=limit[1];i++)
  {
   sprintf(TabVVar[TabArg[0]],"%d",i);
   ExecBloc((Bloc*)TabArg[NbArg-1]);
  }

 free(arg[0]);
 free(arg[1]);
}

/* Instruction While */
static void While (int NbArg,long *TabArg)
{
 char *arg[3],*str;
 int i;
 int Loop=1;
 int IdFuncComp;

 while (Loop)
 {
  i=0;
  arg[0]=CalcArg(TabArg,&i);
  i++;
  str=CalcArg(TabArg,&i);
  IdFuncComp=atoi(str);
  free(str);
  i++;
  arg[1]=CalcArg(TabArg,&i);

  Loop=TabComp[IdFuncComp](arg[0],arg[1]);
  if (Loop) ExecBloc((Bloc*)TabArg[NbArg-1]);
  free(arg[0]);
  free(arg[1]);
 }
}


static void WriteToFile (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2],str[50],*tempstr,*home,*file;
 FILE *f;
 char StrBegin[100];
 char StrEnd[10];
 size_t  size;
 char *buf;
 int maxsize=32000;
 int CurrPos=0,CurrPos2;
 int OldPID;

 arg[0]=CalcArg(TabArg,&i);
 arg[1]=(char*)calloc(1,256);
 for (i=1;i<NbArg;i++)
 {
  tempstr=CalcArg(TabArg,&i);
  arg[1]=strcat(arg[1],tempstr);
  free(tempstr);
 }
 if (arg[1][strlen(arg[1])-1]!='\n')
 {
  i=strlen(arg[1]);
  arg[1]=(char*)realloc(arg[1],strlen(arg[1])+2);
  arg[1][i]='\n';
  arg[1][i+1]='\0';
 }

 sprintf(StrEnd,"#end\n");
 sprintf(StrBegin,"#%s,",ScriptName);

 buf=(char*)calloc(1,maxsize);

 if (arg[0][0]!='/')
 {
  file=strdup(arg[0]);
  home=getenv("HOME");
  arg[0]=(char*)realloc(arg[0],sizeof(char)*(strlen(arg[0])+4+strlen(home)));
  sprintf(arg[0],"%s/%s",home,file);
/*  free(home);*/		/* BUG */
  free(file);
 }
 f=fopen(arg[0],"a+");
 fseek(f,0,SEEK_SET);
 size=fread(buf,1,maxsize,f);
 while(((strncmp(StrBegin,&buf[CurrPos],strlen(StrBegin)))!=0)&&(CurrPos<size))
  CurrPos++;
 if (CurrPos==size)
 {
  sprintf(buf,"%s\n%s%d\n",buf,StrBegin,getpid());
  sprintf(buf,"%s%s",buf,arg[1]);
  sprintf(buf,"%s%s\n",buf,StrEnd);
 }
 else
 {
  sscanf(&buf[CurrPos+strlen(StrBegin)],"%d",&OldPID);
  if (OldPID==getpid())
  {
   sprintf(str,"%d\n",OldPID);
   while(((strncmp(StrEnd,&buf[CurrPos],strlen(StrEnd)))!=0)&&(CurrPos<size))
    CurrPos++;
   memmove(&buf[CurrPos+strlen(arg[1])],&buf[CurrPos],strlen(buf)-CurrPos);
   memmove(&buf[CurrPos],arg[1],strlen(arg[1]));
  }
  else				/* Remplacement des anciennes commandes */
  {
   CurrPos=CurrPos+strlen(StrBegin);
   CurrPos2=CurrPos;
   while(((strncmp(StrEnd,&buf[CurrPos2],strlen(StrEnd)))!=0)&&(CurrPos2<size))
    CurrPos2++;
   sprintf(str,"%d\n%s",getpid(),arg[1]);
   memmove(&buf[CurrPos+strlen(str)],&buf[CurrPos2],strlen(buf)-CurrPos2);
   buf[strlen(buf)-((CurrPos2-CurrPos)-strlen(str))]='\0';
   memmove(&buf[CurrPos],str,strlen(str));
  }
 }

 fclose(f);
 f=fopen(arg[0],"w");
 if (f==NULL)
 {
  fprintf(stderr,"Enable to open file %s\n",arg[0]);
  return;
 }
 fwrite(buf,1,strlen(buf),f);
 fclose(f);

 free(arg[0]);
 free(arg[1]);
}

static void SendToScript (int NbArg,long *TabArg)
{
 char *tempstr,*Msg,*R;
 int dest;
 int j=0;
 Atom myatom;

 /* Calcul destinataire */
 tempstr=CalcArg(TabArg,&j);
 dest=(int)atoi(tempstr);
 free(tempstr);

 /* Calcul contenu */
 Msg=(char*)calloc(256,sizeof(char));
 for (j=1;j<NbArg;j++)
 {
  tempstr=CalcArg(TabArg,&j);
  Msg=(char*)realloc((void*)Msg,strlen(Msg)+strlen(tempstr)+1);
  Msg=strcat(Msg,tempstr);
  free(tempstr);
 }

 /* Calcul recepteur */
 R=(char*)calloc(strlen(x11base->TabScriptId[dest])+1,sizeof(char));
 sprintf(R,"%s",x11base->TabScriptId[dest]);
 myatom=XInternAtom(dpy,R,True);

 if ((BuffSend.NbMsg<40)&&(XGetSelectionOwner(dpy,myatom)!=None))
 {
  /* Enregistrement dans le buffer du message */
  BuffSend.TabMsg[BuffSend.NbMsg].Msg=Msg;
  /* Enregistrement dans le buffer du destinataire */
  BuffSend.TabMsg[BuffSend.NbMsg].R=R;
  /* Enregistrement du message */
  BuffSend.NbMsg++;

  /* Reveil du destinataire */
  XConvertSelection(dpy,XInternAtom(dpy,x11base->TabScriptId[dest],True)
			,propriete,propriete,x11base->win,CurrentTime);
 }
 else
 {
  fprintf(stderr,"Too many messages sended\n");
  free(Msg);
 }
}

/****************************************************/
/* Fonction d'initialisation de TabCom et TabFunc   */
/****************************************************/
void InitCom()
{
 /* commande */
 TabCom[1]=Exec;
 TabCom[2]=HideObj;
 TabCom[3]=ShowObj;
 TabCom[4]=ChangeValue;
 TabCom[5]=ChangePos;
 TabCom[6]=ChangeSize;
 TabCom[7]=ChangeIcon;
 TabCom[8]=ChangeTitle;
 TabCom[9]=ChangeFont;
 TabCom[10]=ChangeForeColor;
 TabCom[11]=SetVar;
 TabCom[12]=SendSign;
 TabCom[13]=Quit;
 TabCom[14]=IfThen;
 TabCom[15]=Loop;
 TabCom[16]=While;
 TabCom[17]=WarpPointer;
 TabCom[18]=WriteToFile;
 TabCom[19]=ChangeBackColor;
 TabCom[21]=ChangeValueMax;
 TabCom[22]=ChangeValueMin;
 TabCom[23]=SendToScript;
 TabCom[24]=ChangeColorset;

 /* Fonction */
 TabFunc[1]=FuncGetValue;
 TabFunc[2]=FuncGetTitle;
 TabFunc[3]=FuncGetOutput;
 TabFunc[4]=FuncNumToHex;
 TabFunc[5]=FuncHexToNum;
 TabFunc[6]=FuncAdd;
 TabFunc[7]=FuncMult;
 TabFunc[8]=FuncDiv;
 TabFunc[9]=FuncStrCopy;
 TabFunc[10]=LaunchScript;
 TabFunc[11]=GetScriptFather;
 TabFunc[12]=ReceivFromScript;
 TabFunc[13]=RemainderOfDiv;
 TabFunc[14]=GetTime;
 TabFunc[15]=GetScriptArg;
 TabFunc[16]=FuncGetFore;
 TabFunc[17]=FuncGetBack;
 TabFunc[18]=FuncGetHili;
 TabFunc[19]=FuncGetShad;
 TabFunc[20]=FuncGetMinValue;
 TabFunc[21]=FuncGetMaxValue;
 /* Fonction de comparaison */
 TabComp[1]=Inf;
 TabComp[2]=InfEq;
 TabComp[3]=Equal;
 TabComp[4]=SupEq;
 TabComp[5]=Sup;
 TabComp[6]=Diff;

}
