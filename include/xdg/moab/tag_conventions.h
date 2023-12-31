// Borrowed from MOAB
#ifndef _XDG_MOABTAG_CONVENTIONS_H
#define XDG_MOAB_TAG_CONVENTIONS_H

// TODO: Rely on installed version of MBTagConventions.hpp

/* GEOM_DIMENSION tag:
 * Represents entities "owned" by a given topological entity in a geometric model
 * size = sizeof(int)
 * type = int
 * value = dimension of geom entity
 * default value = -1
 */
#define GEOM_DIMENSION_TAG_NAME "GEOM_DIMENSION"

/* GLOBAL_ID tag:
 * Represents global id of entities (sets or mesh entities); this id is different than the id
 * embedded in the entity handle
 * size = sizeof(int)
 * type = int
 * value = global id
 * default value = 0 // not -1 to allow gids stored in unsigned data types
 */
#define GLOBAL_ID_TAG_NAME "GLOBAL_ID"

/* CATEGORY tag:
 * String name indicating generic "category" if the entity to which it is assigned (usually
 * sets); used e.g. to indicate a set represents geometric vertex/edge/face/region,
 * dual surface/curve, etc.
 * size = CATEGORY_TAG_NAME_LENGTH (defined below)
 * type = char[CATEGORY_TAG_NAME_LENGTH]
 * value = NULL-terminated string denoting category name
 */
#define CATEGORY_TAG_NAME "CATEGORY"
#define CATEGORY_TAG_SIZE 32

/* NAME tag:
 * A fixed length NULL-padded string containing a name.
 * All values should be assumed to be of type char[NAME_TAG_SIZE].
 * The string need not be null terminated.  All values used for
 * storing or searching for a value must be padded with '\0' chars.
 */
#define NAME_TAG_NAME "NAME"
#define NAME_TAG_SIZE 32

/** \brief Global identifier for interface mesh
 *
 * An integer identifier common to the corresponding mesh entity
 * instances on each processor for a mesh entity on the interface.
 */
#define PARALLEL_GID_TAG_NAME "GLOBAL_ID"


/** \brief Tag used for relationship between surfaces and their parent volumes
 // Tag name used for saving sense of faces in volumes.
// We assume that the surface occurs in at most two volumes.
// Code will error out if more than two volumes per surface.
// The tag data is a pair of tag handles, representing the
// forward and reverse volumes, respectively.  If a surface
// is non-manifold in a single volume, the same volume will
// be listed for both the forward and reverse slots.
*/

#define GEOM_SENSE_2_TAG_NAME "GEOM_SENSE_2"


#endif
