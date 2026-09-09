#ifndef JOB_H
#define JOB_H

#include <stddef.h>

typedef struct Job Job;

/**
 * Creates a new job with the specified parameters.
 *
 * Parameters:
 * - id: The unique identifier for the job.
 * - type: A string representing the type of the job.
 * - payload: A pointer to the payload data for the job.
 * - payload_size: The size of the payload data in bytes.
 * 
 * Ownership:
 * - The caller owns the returned Job.
 * - The caller must eventually call job_destroy().
 * 
 * Returns:
 * - A pointer to the newly created Job structure, or NULL if memory allocation fails.
 */
Job *create_job(int id, const char *type, void *payload, size_t payload_size);
/**
 * Destroys a job and frees its associated memory.
 * 
 * Parameters:
 * - job: A pointer to the Job structure to be destroyed.
 */
void destroy_job(Job *job);

/**
 * Retrieves the unique identifier of a job.
 * Parameters:
 * - job: A pointer to the Job structure.
 * Returns:
 * - The unique identifier of the job.
 */
int get_job_id(const Job *job);
/**
 * Retrieves the type of a job.
 * Parameters:
 * - job: A pointer to the Job structure.
 * Returns:
 * - A pointer to the type string of the job.
 */
const char *get_job_type(const Job *job);
/**
 * Retrieves the payload data of a job.
 * Parameters:
 * - job: A pointer to the Job structure.
 * Returns:
 * - A pointer to the payload data of the job.
 */
void *get_job_payload(const Job *job);
/**
 * Retrieves the size of the payload data of a job.
 * Parameters:
 * - job: A pointer to the Job structure.
 * Returns:
 * - The size of the payload data in bytes.
 */
size_t get_job_payload_size(const Job *job);

#endif // JOB_H