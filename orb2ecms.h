#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>

#include "coords.h"
#include "tr.h"
#include "orb.h"
#include "db.h"
#include "xtra.h"
#include "db_origin.h"

#include "site_read.h"

Tbl *ebs_tbl;
Tbl *ecms_tbl;

