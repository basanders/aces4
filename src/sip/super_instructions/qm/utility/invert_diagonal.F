      subroutine invert_diagonal(
     * array_0, rank_0, index_values_0, size_0, extents_0, data_0,
     * array_1, rank_1, index_values_1, size_1, extents_1, data_1,
     * ierr) BIND(C)
      use, intrinsic :: ISO_C_BINDING
      implicit none

c--------------------------------------------------------------------------
c Usage: execute invert_diagonal T1 T2
C returns element by element t1= t1/t2
c
c--------------------------------------------------------------------------
      include 'sip_interface.f'

! output array --> array_0
      integer(C_INT), intent(in)::array_0
      integer(C_INT), intent(in)::rank_0
      integer(C_INT), intent(in)::index_values_0(1:rank_0)
      integer(C_INT), intent(in)::size_0
      integer(C_INT), intent(in)::extents_0(1:rank_0)
      real(C_DOUBLE), intent(out)::data_0(1:size_0)

      integer(C_INT), intent(in)::array_1
      integer(C_INT), intent(in)::rank_1
      integer(C_INT), intent(in)::index_values_1(1:rank_1)
      integer(C_INT), intent(in)::size_1
      integer(C_INT), intent(in)::extents_1(1:rank_1)
      real(C_DOUBLE), intent(out)::data_1(1:size_1)

      integer(C_INT), intent(out)::ierr

      if(rank_0 .ne. rank_1) then
	  write(*,*) "invert diagonal ranks not same"
	  call abort_job()
      endif

      if (rank_0 .eq. 3) then
      call do_return_inv3(1, extents_0(1),
     *                    1, extents_0(2),
     *                    1, extents_0(3),
     *                    data_0,data_1,ierr)
      elseif (rank_0 .eq. 5) then
      call do_return_inv5(1, extents_0(1),
     *                    1, extents_0(2),
     *                    1, extents_0(3),
     *                    1, extents_0(4),
     *                    1, extents_0(5),
     *                    data_0,data_1,ierr)
      else
	  write(*,*) "bad call to invert_diagonal"
	  call abort_job()
      endif

      if (ierr .eq. 1) then
	  call abort_job()
      endif

      return
      end

      subroutine do_return_inv3( a1, a2, b1, b2, c1, c2,
     *                           array1,array2,ierr)
      implicit none
      integer ierr
      integer a, a1, a2
      integer b, b1, b2
      integer c, c1, c2 
      double precision array1(a1:a2,b1:b2,c1:c2) 
      double precision array2(a1:a2,b1:b2,c1:c2) 
      double precision val, div

      val = 0.0 

      do c = c1,c2
      do b = b1,b2
      do a = a1,a2
          div = array2(a,b,c)
C	  if (div .eq. 0.d0) then
C	      write(*,*) "--ZERO denominator in invert diagonal"
C	      write(*,100) a,b,c,div
C	      ierr = 1
C	      return
C          endif
          if (div .ne. 0.d0) then
	      val = array1(a,b,c)/div
	      array1(a,b,c) = val
	  endif
C          write(6,100) a,b,c,array1(a,b,c),array2(a,b,c),val
      enddo 
      enddo 
      enddo 
  100 format(3I3,24E14.6)

      return
      end

      subroutine do_return_inv5(a1,a2,b1,b2,c1,c2,d1,d2,e1,e2, 
     *                          array1,array2,ierr)
      implicit none
      integer ierr
      integer a, a1, a2
      integer b, b1, b2
      integer c, c1, c2 
      integer d, d1, d2 
      integer e, e1, e2
      double precision array1(a1:a2,b1:b2,c1:c2,d1:d2,e1:e2) 
      double precision array2(a1:a2,b1:b2,c1:c2,d1:d2,e1:e2) 
      double precision val, div

      val = 0.0 

      do e = e1,e2
      do d = d1,d2
      do c = c1,c2
      do b = b1,b2
      do a = a1,a2
          div = array2(a,b,c,d,e)
C	  if (div .eq. 0.d0) then
C	      write(*,*) "--ZERO denominator in invert diagonal"
C	      write(*,100) a,b,c,d,e,div
C	      ierr = 1
C	      return
C          endif
          if (div .ne. 0.d0) then
	      val = array1(a,b,c,d,e)/div
	      array1(a,b,c,d,e) = val
	  endif
C          write(6,100) a,b,c,array1(a,b,c),array2(a,b,c),val
      enddo 
      enddo 
      enddo 
      enddo 
      enddo 
  100 format(5I3,24E14.6)

      return
      end
