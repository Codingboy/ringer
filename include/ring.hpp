#ifndef RING_HPP
#define RING_HPP

#ifndef MAPSIZE
#define MAPSIZE (256)
#endif

#ifndef IVSIZE
#define IVSIZE (128)
#endif

//#define KNOWNPLAINTEXTATTACK
//#define NDEBUG

class Ring
{
	public:
		/**
		 * Creates a Ring.
		 * @param[in] c password
		 * @param[in] length length of c
		 * @param[in] pos salt (may be public)
		 * @param[in] posLength length of pos (salt)
		 * In encodemode this should be set to a random value.
		 * @param[in] mutationInterval number of operations until ring is mutated (may be public)
		 * If you want no mutation set it to 0.
		 * @warning Decreasing mutationInterval too much may cause low performance!
		 * @pre c is allocated for length characters
		 * @pre length > 0
		 * @pre length <= MAPSIZE
		 * @pre pos is allocated for posLength characters
		 * @pre posLength > 0
		 * @pre mutationInterval%IVSIZE == 0 if MULTICORE is defined
		 * @note Define MULTICORE to make Ring multithread compatible. If MULTICORE is defined, you have to call shuffle() manually and by this mutating the ring. You also should use the posIndex correctly.
		 */
		Ring(const unsigned char* c, unsigned int length, const unsigned char* pos, unsigned int posLength, unsigned int mutationInterval);
		/**
		 * Deletes a Ring.
		 */
		virtual ~Ring();
		/**
		 * Reinitialises a Ring.
		 * @note This is needed between 2 independent encode/decode operations (e.g. 2 files).
		 * @post Ring in post constructed status
		 */
		void reinit();
		/**
		 * Encodes a single character.
		 * @param[in] c character to encode
		 * @param[in] posIndex points to the salt used to encode the byte
		 * @pre posIndex < IVSIZE if MULTICORE is defined
		 * @return encoded character
		 */
		unsigned char encode(unsigned char c);
		/**
		 * Decodes a single character.
		 * @param[in] c character to decode
		 * @param[in] posIndex points to the salt used to decode the byte
		 * @pre posIndex < IVSIZE if MULTICORE is defined
		 * @return decoded character
		 */
		unsigned char decode(unsigned char c);
		/**
		 * Encodes an array of characters inplace.
		 * @param[in,out] c array of characters to encode
		 * @param[in] posIndex points to the salt used to encode the byte
		 * @pre posIndex < IVSIZE if MULTICORE is defined
		 * @pre c is allocated for length characters
		 * @pre length > 0
		 * @post c contains encoded characters
		 */
		void encode(unsigned char* c, unsigned int length);
		/**
		 * Decodes an array of characters inplace.
		 * @param[in,out] c array of characters to decode
		 * @param[in] posIndex points to the salt used to decode the byte
		 * @pre posIndex < IVSIZE if MULTICORE is defined
		 * @pre c is allocated for length characters
		 * @pre length > 0
		 * @post c contains decoded characters
		 */
		void decode(unsigned char* c, unsigned int length);
#ifdef MULTICORE
		/**
		 * Mutates the map independent from the key.
		 * This method is used to mutate the ring after mutationInterval operations.
		 */
		void shuffle();
		unsigned int mutationInterval;
		unsigned int operationsSinceMutation;
#endif
#ifdef KNOWNPLAINTEXTATTACK
		unsigned char map[MAPSIZE];
		unsigned char decodeMap[MAPSIZE];
		bool mapSpecified[MAPSIZE];
#endif
	protected:
		unsigned char last;
#ifndef MULTICORE
		/**
		 * Mutates the map independent from the key.
		 * This method is used to mutate the ring after mutationInterval operations.
		 */
		void shuffle();
		unsigned int mutationInterval;
		unsigned int operationsSinceMutation;
#endif
		/**
		 * Mutates the map depending on the key.
		 * This method is used to initialise the ring.
		 * @deprecated This method is only in use for initialisation of the ring. All other mutations are done with shuffle() because this method needs to much time.
		 */
		void mutate();
#ifndef KNOWNPLAINTEXTATTACK
		unsigned char map[MAPSIZE];
		unsigned char decodeMap[MAPSIZE];
#endif
		unsigned char pos[IVSIZE];
		unsigned int posLength;
		unsigned char initPos[IVSIZE];
		unsigned int actualPos;
		unsigned char pw[MAPSIZE];
		unsigned int pwLength;
	private:
};

#endif