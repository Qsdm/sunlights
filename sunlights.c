/* Libraries borrowed by sunwait open source program */
/* written as an example by Qsdm Feb, 2014 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <wiringPi.h>
#include "sunriset.h"
#include <curl/curl.h>


print_usage() {
  printf("usage: RUNLIGHTS [latitude] [longitude] [+/-offset in minutes] [duration file]  \n\n");
  printf("latitude/longitude are expressed in floating-point degrees \n");
  printf("offset is the minutes after sunrise to turn on (negative for before) \n");
  printf("duration file contains 12 lines indicating duration to stay on each month, in minutes \n");
  printf("example: lightson 38.794433N 77.069450W -5 120 durations.ini \n");
  printf("This example will wait until 5 minutes before sunset then turn on lights for the amount indicated in the ini file where Lat/Long is locating Alexandria, VA\n");
  printf("Make sure your system has the correct time and timezone set\n");
}


  /*  This routine handles the on/off of the relay circuit  */
turnOnRelay(int duration,int pin) {
  wiringPiSetup () ;
  pinMode (pin, OUTPUT) ;
  digitalWrite (pin, HIGH);
  printf("==> Lights ON \n");
  printf("==> Waiting:  %i minutes \n",duration);
  delay((unsigned int) duration*1000*60);
  digitalWrite (pin, LOW) ;
  printf("==> Lights OFF \n");
}


  /*  This routine handles the on/off of the OpenSprinkler Zone  */
turnOnZone(int duration,int zone) {
	printf("==> Turning Lights ON \n");
	CURL *curl;
  	CURLcode res;
	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8080/sn16=1&t=0");
 
    		/* Perform the request, res will get the return code */ 
    		res = curl_easy_perform(curl);
    		/* Check for errors */ 
    		if(res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
              	else {
              		printf("==> Lights ON \n");
              		delay((unsigned int) duration*1000*60);
              		printf("==> Turning Lights OFF \n");
              		curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8080/sn16=0");
              		res = curl_easy_perform(curl);
              		if(res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
              		else printf("==> Lights OFF \n");
              		delay((unsigned int) 2*1000*60);
              	}
 
    		/* always cleanup */ 
    		curl_easy_cleanup(curl);
  	}
}
  
  
  

  /*  This reads the INI file to get the duration the lights should be on  */
int getDurationByMonth(int currentmonth, char* filepath) {
   char c[10];
   FILE * f ;
   int i;
   int output=-1;
   if ((f=fopen(filepath, "r"))==NULL) {
        fprintf(stderr, "error: cannot open %s\n", filepath);
        exit(-2);
   }
   char* month;
   char* val;
   for (i=1 ; i<=12 && fgets(c, sizeof(c), f)!= NULL ; i++) {
        if(i==currentmonth) output=atoi(c);
   }
   fclose(f);
   printf("==> today's duration is: %i \n",output);
   return output;
}

const int pin=15;
const char* timezone_name;
long int timezone_offset;
 
int main(int argc, char *argv[]) {
  double lon;
  double lat;
  int year,month,day;
  int offset_min,duration;
  int sit;
  char* durationFile;
  double up, down, now, interval, offset;

  time_t tt;
  struct tm *tm;

    /*  5  inputs plus the command is needed to run */
  if(argc!=5) {
      print_usage();
      exit(-1);
  }
  
  while (1) {
  	tt = time(NULL);
  	ctime(&tt);
  	tm = localtime(&tt);
  
         /* get inputs passed in via command line */
       lat=strtod(argv[1],NULL);
       lon=strtod(argv[2],NULL);
       offset_min=atoi(argv[3]);
       durationFile=argv[4];
     
     
         /*  Setup time */
       year = 1900 + tm->tm_year;
       month = 1+ tm->tm_mon;
       day =  1+ tm->tm_mday;
       timezone_name = tm->tm_zone;
       timezone_offset = tm->tm_gmtoff;
     
         /*  get sunrise and sunset values  */
       sit = sun_rise_set( year, month, day, lon, lat, &up, &down );
       up = TMOD(up+timezone_offset/3600);
       down = TMOD(down+timezone_offset/3600);
       printf ("==> Sunrise: %5.2f, Sunset: %5.2f GMT\n", up, down);
       
       printf("get Duration values from file %s \n",durationFile);
       duration=getDurationByMonth(month,durationFile);
         
       if (0 != sit) {
           fprintf(stderr, "Event does not occur today\n");
           exit(1);
       }
       now = tm->tm_hour/1.0 + tm->tm_min/60.0 + tm->tm_sec/3600.0; 
       offset = offset_min/60.0; 
       interval = down - now + offset;
    
       if (0 > interval) {
           printf("==>Sunset has occured today  %5.2f hours ago\n",interval);
       	   if (interval+(duration/60)>0 ) {
       	   	duration=duration+(interval*60);
       		printf("Can wait %i minutes\n",duration);
       	   }
       	   else interval=interval+24.0;
	}
     
       printf ("==> Hours to Wait: %5.2f \n", interval);
       delay((unsigned int)(interval*3600.0*1000));
       
       /* do */
       turnOnZone(duration,pin);
       printf("==>SUNLIGHTS program complete for today");
  }
}




