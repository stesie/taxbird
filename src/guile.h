/* Copyright(C) 2004,05 Stefan Siegl <ssiegl@gmx.de>
 * taxbird - free program to interface with German IRO's Elster/Coala
 *
 * This program is free software; you can redistribute it and/or modify
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

#ifndef TAXBIRD_GUILE_H
#define TAXBIRD_GUILE_H

#define scm_c_lookup_ref(a) SCM_VARIABLE_REF(scm_c_lookup(a))

/* initialize taxbird's guile backend */
void taxbird_guile_init(void);

/* evaluate given file, checking it's signature */
int taxbird_guile_eval_file(const char *fn);



#endif /* TAXBIRD_GUILE_H */
