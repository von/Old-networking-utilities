/*
	atoval()

	Basic idea for this routine shamelessly stolen from nettest

	Given a numeric string returns a integer. Correctly handles 'k', 'K',
	'm' and 'M'.
*/
	
atoval(s)
register char *s;
{
	register int retval;

	retval = atoi(s);

	while (isdigit(*s))
		++s;

	if (*s == 'k' || *s == 'K')
		retval <<= 10;
	if (*s == 'm' || *s == 'M')
		retval <<= 20;

	return(retval);
}
