#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <uuid/uuid.h>


const unsigned int MAX_GROUP_NUMBER = 32;

int main(int argc, char* argv[])
{
    uid_t u_id = getuid();
    struct passwd* u_pass = getpwuid(u_id);
    printf("uid=%d(%s) ", u_id, u_pass->pw_name);

    gid_t u_gid = getgid();
    struct group* u_group = getgrgid(u_gid);
    printf("gid=%d(%s) ", u_gid, u_group->gr_name);

    gid_t group_list[MAX_GROUP_NUMBER] = {};
    int n_groups = getgroups(MAX_GROUP_NUMBER, group_list);

    if(!n_groups)
        return 0;

    printf("%d(%s)", group_list[0], getgrgid(group_list[0])->gr_name);
    for(int i = 1; i < n_groups; i++)
        printf(",%d(%s)", group_list[i], getgrgid(group_list[i])->gr_name);

    return 0;
}
