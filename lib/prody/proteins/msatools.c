/* ProDy: A Python Package for Protein Dynamics Analysis
 *
 * Copyright (C) 2010-2012 Ahmet Bakan
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * Author: Ahmet Bakan
 * Copyright (C) 2010-2012 Ahmet Bakan
 */

#include "Python.h"
#include "numpy/arrayobject.h"

const int twenty[20] = {1, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 
                        14, 16, 17, 18, 19, 20, 22, 23, 25};

static PyObject *parseFasta(PyObject *self, PyObject *args) {

    /* Parse sequences from *filename* into the memory pointed by the
       Numpy array passed as Python object.  This function assumes that
       the sequences are aligned, i.e. have same number of lines at equal
       lengths. */

    char *filename;
    PyArrayObject *msa;
    
    if (!PyArg_ParseTuple(args, "sO", &filename, &msa))
        return NULL;
    
    long i = 0, lenseq = msa->dimensions[1];
    long lenline = 0, lenlast = 0, numlines = 0; 
    long size = lenseq + 100, iline = 0;
    char line[size];

    FILE *file = fopen(filename, "rb");
    while (fgets(line, size, file) != NULL) {
        if (line[0] == '>')
            continue;

        for (i = 0; i < strlen(line); i++)
            if (line[i] == ' ' || line[i] == '\n')
                break;
        lenline = i;
        lenlast = lenseq % lenline;
        numlines = (lenseq - lenlast) / lenline;
        break;
    }
    
    fseek(file, 0, SEEK_SET);

    int slash = 0, dash = 0, j = 0;
    long index = 0, ccount = 0;
    char *data = (char *)PyArray_DATA(msa);
    char clabel[size], ckey[size];
    PyObject *labels, *dict, *plabel, *pkey, *pcount;
    labels = PyList_New(0);
    dict = PyDict_New();

    while (fgets(line, size, file) != NULL) {
        iline++;
        if (line[0] != '>')
            continue;
        for (i = 1; i < size; i++)
            if (line[i] != ' ')
                break;
        strcpy(line, line + i);

        /* parse label */
        slash = 0;
        dash = 0;
        for (i = 0; i < size; i++)
            if (line[i] == '\n' || line[i] == ' ') 
                break;
            else if (line[i] == '/' && slash == 0 &&  dash == 0)
                slash = i;
            else if (line[i] == '-' && slash > 0 && dash == 0)
                dash = i;
        
        if (slash > 0 && dash > slash) {
            strncpy(ckey, line, slash);
            strncpy(clabel, line, i);
            
            clabel[i] = '\0';
            ckey[slash] = '\0';
            pkey = PyString_FromString(ckey);
            plabel = PyString_FromString(clabel);
            pcount = PyInt_FromLong(ccount);
            if (plabel == NULL || pcount == NULL ||
                PyList_Append(labels, plabel) < 0 ||
                PyDict_SetItem(dict, pkey, pcount)) {
                PyErr_SetString(PyExc_IOError, 
                                "failed to parse msa, at line");
                Py_XDECREF(pcount);
                Py_XDECREF(plabel);
                Py_XDECREF(pkey);
                return NULL;
            }
            Py_DECREF(pkey);
            Py_DECREF(plabel);
            Py_DECREF(pcount); 
        } else {
            strncpy(clabel, line, i);
            clabel[i] = '\0';
            plabel = PyString_FromString(clabel);
            pcount = PyInt_FromLong(ccount);
            if (plabel == NULL || pcount == NULL ||
                PyList_Append(labels, plabel) < 0 ||
                PyDict_SetItem(dict, plabel, pcount)) {
                PyErr_SetString(PyExc_IOError, 
                                "failed to parse msa, at line");
                Py_XDECREF(pcount);
                Py_XDECREF(plabel);
                return NULL;
            }
            Py_DECREF(plabel);
            Py_DECREF(pcount);
         }

        
        /* parse sequence */
        
        for (i = 0; i < numlines; i++) {
            if (fgets(line, size, file) == NULL) {
                PyErr_SetString(PyExc_IOError, 
                                "failed to parse msa, at line");
                return NULL;
            }
            for (j = 0; j < lenline; j++)
                data[index++] = line[j];
        }
        
        if (lenlast) {
            if (fgets(line, size, file) == NULL) {
                PyErr_SetString(PyExc_IOError, 
                                "failed to parse msa, at line");
                return NULL;
            }
            for (j = 0; j < lenlast; j++)
                data[index++] = line[j];
        }
        ccount++;
    }

    fclose(file);
    PyObject *result = Py_BuildValue("(OO)", labels, dict);
    Py_DECREF(labels);
    Py_DECREF(dict);
    return result;
}


static PyObject *parseSelex(PyObject *self, PyObject *args) {

    /* Parse sequences from *filename* into the the memory pointed by the
       Numpy array passed as Python object.  This function assumes that
       the sequences are aligned, i.e. start and end at the same column. */

    char *filename;
    PyArrayObject *msa;
    
    if (!PyArg_ParseTuple(args, "sO", &filename, &msa))
        return NULL;

    long i = 0, beg = 0, end = 0, lenseq = msa->dimensions[1]; 
    long size = lenseq + 100, iline = 0;
    char line[size];

    FILE *file = fopen(filename, "rb");
    while (fgets(line, size, file) != NULL) {
        iline++;
        if (line[0] == '#' || line[0] == '/' || line[0] == '%')
            continue;

        for (i = 0; i < size; i++)
            if (line[i] == ' ')
                break;
        for (; i < size; i++)
            if (line[i] != ' ')
                break;
        beg = i;
        end = beg + lenseq;
        break;
    }
    iline--;
    fseek(file, - strlen(line), SEEK_CUR);

    int slash = 0, dash = 0;
    long index = 0, ccount = 0;
    char *data = (char *)PyArray_DATA(msa);
    char clabel[beg], ckey[beg];
    PyObject *labels, *dict, *plabel, *pkey, *pcount;
    labels = PyList_New(0);
    dict = PyDict_New();

    int space = beg - 1; /* index of space character before sequence */
    while (fgets(line, size, file) != NULL) {
        iline++;
        if (line[0] == '#' || line[0] == '/' || line[0] == '%')
            continue;
            
        if (line[space] != ' ') {
            PyErr_SetString(PyExc_IOError, 
                            "failed to parse msa, at line");
            return NULL;
        } 

        /* parse label */
        
        slash = 0;
        dash = 0;
        for (i = 0; i < size; i++)
            if (line[i] == ' ') 
                break;
            else if (line[i] == '/' && slash == 0 &&  dash == 0)
                slash = i;
            else if (line[i] == '-' && slash > 0 && dash == 0)
                dash = i;
        if (slash > 0 && dash > slash) {
            strncpy(ckey, line, slash);
            strncpy(clabel, line, i);
            clabel[i] = '\0';
            ckey[slash] = '\0';
            pkey = PyString_FromString(ckey);
            plabel = PyString_FromString(clabel);
            pcount = PyInt_FromLong(ccount);
            if (!plabel || !pcount || 
                PyList_Append(labels, plabel) < 0 ||
                PyDict_SetItem(dict, pkey, pcount)) {
                PyErr_SetString(PyExc_IOError, 
                                "failed to parse msa, at line");
                Py_XDECREF(pcount);
                Py_XDECREF(plabel);
                Py_XDECREF(pkey);
                return NULL;
            }
            Py_DECREF(pkey);
            Py_DECREF(plabel);
            Py_DECREF(pcount);            
        } else {
            strncpy(clabel, line, i);
            clabel[i] = '\0';
            plabel = PyString_FromString(clabel);
            pcount = PyInt_FromLong(ccount);
            if (!plabel || !pcount ||
                PyList_Append(labels, plabel) < 0 ||
                PyDict_SetItem(dict, plabel, pcount)) {
                PyErr_SetString(PyExc_IOError, 
                                "failed to parse msa, at line");
                Py_XDECREF(pcount);
                Py_XDECREF(plabel);
                return NULL;
            }
            Py_DECREF(plabel);
            Py_DECREF(pcount);
         }
        
        /* parse sequence */
        for (i = beg; i < end; i++)
            data[index++] = line[i];
        ccount++;
    }
    fclose(file);

    PyObject *result = Py_BuildValue("(OO)", labels, dict);
    Py_DECREF(labels);
    Py_DECREF(dict);
    return result;
}


static PyObject *calcShannonEntropy(PyObject *self, PyObject *args,
                                    PyObject *kwargs) {

    PyArrayObject *msa, *entropy;
    int dividend = 0;
    
    static char *kwlist[] = {"msa", "entropy", "dividend", NULL};
        
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|i", kwlist,
                                     &msa, &entropy, &dividend))
        return NULL;
    
    long numseq = msa->dimensions[0], lenseq = msa->dimensions[1];
   
    if (entropy->dimensions[0] != lenseq) {
        PyErr_SetString(PyExc_IOError, 
                        "msa and entropy array shapes do not match");
        return NULL;
    }

    char *seq = (char *) PyArray_DATA(msa);
    double *ent = (double *) PyArray_DATA(entropy);

    /* start here */
    long size = numseq * lenseq; 
    double count[128]; /* number of ASCII characters*/
    double shannon = 0, probability = 0, numgap = 0, denom = numseq;
    long i = 0, j = 0;
    
    double ambiguous = 0;
    int twenty[20] = {65, 67, 68, 69, 70, 71, 72, 73, 75, 76, 
                      77, 78, 80, 81, 82, 83, 84, 86, 87, 89};
    for (i = 0; i < lenseq; i++) {

        /* zero counters */
        for (j = 65; j < 91; j++)
            count[j] = 0;
        for (j = 97; j < 123; j++)
            count[j] = 0;
        
        /* count characters in a column*/
        for (j = i; j < size; j += lenseq)
            count[(int) seq[j]]++;
        for (j = 65; j < 91; j++)
            count[j] += count[j + 32];
        
        /* handle ambiguous amino acids */
        if (count[66]) {
            ambiguous = count[66] / 2.; /* B */
            count[66] = 0;
            count[68] += ambiguous; /* D */
            count[78] += ambiguous; /* N */
        }
        if (count[90]) {
            ambiguous = count[90] / 2.; /* Z */
            count[90] = 0;
            count[69] += ambiguous; /* E */
            count[81] += ambiguous; /* Q */
        }
        if (count[74]) {
            ambiguous = count[74] / 2.; /* J */
            count[74] = 0;
            count[73] += ambiguous; /* I */
            count[76] += ambiguous; /* L */
        }
        if (count[88]) {
            ambiguous = count[88] / 20.; /* X */
            count[88] = 0;
            for (j = 0; j < 20; j++)
                count[twenty[j]] += ambiguous;
        }
        
        /* non-gap counts */
        numgap = numseq;
        for (j = 65; j < 91; j++)
            numgap -= count[j];
        
        shannon = 0;
        denom = numseq;
        if (dividend)
            denom = numseq - numgap;
        else if (numgap > 0) {
            probability = numgap / numseq;
            shannon += probability * log(probability);
        }

        for (j = 65; j < 91; j++) {
            if (count[j] > 0) {
                probability = count[j] / denom;
                shannon += probability * log(probability);
            }
        }
        ent[i] = -shannon;
    }

    Py_RETURN_NONE;
}

static void fixJoint(double *joint[]) {
    
    int k, l;
    double *jrow, jp; 
    for (k = 0; k < 27; k++) {
        jrow = joint[k]; 
        /* B */
        jp = jrow[2];  
        if (jp > 0) {
            jrow[4] = jrow[14] = jp / 2;
            jrow[2] = 0;
        }
        jp = joint[2][k]; 
        if (jp > 0) {
            joint[4][k] = joint[14][k] = jp / 2;
            joint[2][k] = 0;
        }
        /* Z */
        jp = jrow[26]; 
        if (jp > 0) {
            jrow[5] = jrow[17] = jp / 2;
            jrow[26] = 0;
        }
        jp = joint[26][k]; 
        if (jp > 0) {
            joint[5][k] = joint[17][k] = jp / 2;
            joint[26][k] = 0;
        }
        /* J */
        jp = jrow[10]; 
        if (jp > 0) {
            jrow[9] = jrow[12] = jp / 2;
            jrow[10] = 0;
        }
        jp = joint[10][k]; 
        if (jp > 0) {
            joint[9][k] = joint[12][k] = jp / 2;
            joint[10][k] = 0;
        }
        /* X */
        jp = jrow[24]; 
        if (jp > 0) {
            jp = jp / 20.;
            for (l = 0; l < 20; l++)
                jrow[twenty[l]] = jp;    
            jrow[24] = 0;
        }
        jp = joint[24][k]; 
        if (jp > 0) {
            jp = jp / 20.;
            for (l = 0; l < 20; l++)
                joint[twenty[l]][k] = jp;    
            joint[24][l] = 0;
        }
    }
    
}

static void zeroJoint(double *joint[]) {

    int k, l;
    double *jrow;        
    for (k = 0; k < 27; k++) {
        jrow = joint[k];
        for (l = 0; l < 27; l++)
            jrow[l] = 0;
    }
}

static double calcMI(double *joint[], double *probs[], long i, long j) {

    int k, l;
    double *jrow, jp, mi = 0;
    for (k = 0; k < 27; k++) {
        jrow = joint[k];
        for (l = 0; l < 27; l++) {
            jp = jrow[l];
            if (jp > 0)
                mi += jp * log(jp / probs[i][k] / probs[j][l]);
        }
    }
    return mi;
}

static PyObject *calcMutualInfo(PyObject *self, PyObject *args,
                                PyObject *kwargs) {

    PyArrayObject *msa, *mutinfo;
    int ambiguous = 1, debug = 0;
    
    static char *kwlist[] = {"msa", "mutinfo", "ambiguous", "debug", NULL};
        
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|ii", kwlist,
                                     &msa, &mutinfo, &ambiguous, &debug))
        return NULL;

    /* check dimensions */
    long numseq = msa->dimensions[0], lenseq = msa->dimensions[1];
   
    if (mutinfo->dimensions[0] != lenseq || 
        mutinfo->dimensions[1] != lenseq) {
        PyErr_SetString(PyExc_IOError, 
                        "msa and mutinfo array shapes do not match");
        return NULL;
    }
    
    /* get pointers to data */
    
    char *seq = (char *) PyArray_DATA(msa); /*size: numseq x lenseq */
    double *mut = (double *) PyArray_DATA(mutinfo); /*size: lenseq x lenseq */

    /* allocate memory */
    int *iseq = malloc(numseq * sizeof(double));
    if (!iseq) {
        PyErr_SetString(PyExc_MemoryError, "out of memory");
        return NULL;
    }
        
    double **probs;
    probs = malloc(lenseq * sizeof(double*));
    if (!probs) {
        PyErr_SetString(PyExc_MemoryError, "out of memory");
        return NULL;
    }

    double **joint;
    joint = malloc(27 * sizeof(double*));
    if (!joint) {
        free(iseq);
        free(probs);
        PyErr_SetString(PyExc_MemoryError, "out of memory");
        return NULL;
    }
    
    long i, j;
   
    for (i = 0; i < lenseq; i++)  {
        probs[i] = malloc(27 * sizeof(double));  
        if (!probs[i]) {
            for (j = 0; j <= i; j++)
                free(probs[j]);
            free(probs);
            free(iseq);
            PyErr_SetString(PyExc_MemoryError, "out of memory");
            return NULL;
        }
        for (j = 0; j < 27; j++)
            probs[i][j] = 0;
    }

    for (i = 0; i < 27; i++)  {
        joint[i] = malloc(27 * sizeof(double));  
        if (!joint[i]) {
            for (j = 0; j <= i; j++)
                free(joint[j]);
            for (j = 0; j <= lenseq; j++)
                free(probs[j]);
            free(probs);
            free(joint);
            free(iseq);
            PyErr_SetString(PyExc_MemoryError, "out of memory");
            return NULL;
        }
    }

    int a, b;
    long k, l, diff, offset;
    double *prow, p_incr = 1. / numseq, prb = 0;
    i = 0;
    
    /* calculate first row of MI matrix, while calculating probabilities */
    /*time_t t = clock();*/
    for (j = 1; j < lenseq; j++) {

        zeroJoint(joint);
        diff = j - 1;

        for (k = 0; k < numseq; k++) {
            offset = k * lenseq;
            if (diff) {
                a = iseq[k];
            } else {
                a = (int) seq[offset + i];
                if (a > 90)
                    a -= 96;
                else
                    a -= 64;
                if (a < 1 || a > 26)
                    a = 0; /* gap character */
                iseq[k] = a;
            }
            
            b = (int) seq[offset + j];
            if (b > 90)
                b -= 96;
            else
                b -= 64;
            if (b < 1 || b > 26)
                b = 0; /* gap character */
            joint[a][b] += p_incr;
            
            probs[j][b] += p_incr;
            if (!diff)
                probs[i][a] += p_incr;
        }
        
        if (ambiguous) {
            for (k = 0; k < lenseq; k++) {
                prow = probs[k]; 
                if (prow[2] > 0) { /* B -> D, N  */
                    prow[4] = prow[14] = prow[2] / 2.;
                    prow[2] = 0;
                }
                if (prow[10] > 0) { /* J -> I, L  */
                    prow[9] = prow[12] = prow[10] / 2.;
                    prow[10] = 0;
                }
                if (prow[26] > 0) { /* Z -> E, Q  */
                    prow[5] = prow[17] = prow[26] / 2.;
                    prow[26] = 0;
                }
                if (prow[24] > 0) { /* X -> 20 AA */
                    prb = prow[24] / 20.; 
                    for (l = 0; l < 20; l++)
                        prow[twenty[l]] += prb;
                    prow[24] = 0;
                }
            }
            fixJoint(joint);
        }        
        mut[i * lenseq + j] = mut[i + lenseq * j] = calcMI(joint, probs, i, j);
    }
    /*printf("%f\n", (double) (clock()-t)/CLOCKS_PER_SEC);*/
    /* calculate rest of MI matrix */
    /*t = clock();*/
    for (i = 1; i < lenseq; i++) {
        for (j = i + 1; j < lenseq; j++) {
            zeroJoint(joint);
            
            diff = j - i - 1;
            for (k = 0; k < numseq; k++) {
                offset = k * lenseq;
                if (diff) {
                    a = iseq[k];
                } else {
                    a = (int) seq[offset + i];
                    if (a > 90)
                        a -= 96;
                    else
                        a -= 64;
                    if (a < 1 || a > 26)
                        a = 0; /* gap character */
                    iseq[k] = a;
                }
                
                b = (int) seq[offset + j];
                if (b > 90)
                    b -= 96;
                else
                    b -= 64;
                if (b < 1 || b > 26)
                    b = 0; /* gap character */
                joint[a][b] += p_incr;
            }
            if (ambiguous)
                fixJoint(joint);
            mut[i * lenseq + j] = mut[i + lenseq * j] = 
                calcMI(joint, probs, i, j);
        }
    }
    /*printf("%f\n", (double) (clock()-t)/CLOCKS_PER_SEC);*/
    
    /* free memory */
    for (i = 0; i < lenseq; i++){  
        free(probs[i]);
    }  
    free(probs);
    for (i = 0; i < 27; i++){  
        free(joint[i]);
    }  
    free(joint);
    free(iseq);
    
    Py_RETURN_NONE;
}



static PyMethodDef msatools_methods[] = {

    {"parseSelex",  (PyCFunction)parseSelex, METH_VARARGS, 
     "Return list of labels and a dictionary mapping labels to sequences \n"
     "after parsing the sequences into empty numpy character array."},

    {"parseFasta",  (PyCFunction)parseFasta, METH_VARARGS, 
     "Return list of labels and a dictionary mapping labels to sequences \n"
     "after parsing the sequences into empty numpy character array."},

    {"calcShannonEntropy",  (PyCFunction)calcShannonEntropy, 
     METH_VARARGS | METH_KEYWORDS, 
     "Calculate information entropy for given character array into given \n"
     "double array."},

    {"calcMutualInfo",  (PyCFunction)calcMutualInfo, 
     METH_VARARGS | METH_KEYWORDS, 
     "Calculate mutual information for given character array into given \n"
     "2D double array."},
     
    {NULL, NULL, 0, NULL}
    
};


PyMODINIT_FUNC initmsatools(void) {

    Py_InitModule3("msatools", msatools_methods,
        "Multiple sequence alignment IO and analysis tools.");
        
    import_array();
}
