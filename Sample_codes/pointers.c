# include <stdio.h>
# include <pthread.h>
# include <openmp.h>

int main(void)
{

    printf("pointers\n");
    int a = 5;
    printf("%d\n", a);
    printf("memmory address for a = %p\n", &a);

    int *p = &a;
    printf("address of p: %p\n", &p);
    printf("address stored in p: %p\n", p);
    printf("value pointed by p: %i\n", *p);
    printf("Note: *p and &a are the same\n");

    printf("print *&a: %d  and *p: %i\n", *&a, *p);


    

    return 0;
}

