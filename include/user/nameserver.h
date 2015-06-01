#ifndef _NAMESERVER_H
#define _NAMESERVER_H

void nameserverTask();

/** Returns:
0   Success
-1  Name table full
-2  Name server doesn't exist
-3  Generic error
*/
int RegisterAs(char *name);

/** Returns:
>=0  Task id
-1   No task registered under that name
-2   Name server doesn't exist
-3   Generic error
 */
int WhoIs(char *name);


#endif
