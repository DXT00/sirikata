FasdUAS 1.101.10   ��   ��    k             i         I      �� 	���� 0 getparentpath GetParentPath 	  
�� 
 o      ���� 0 thefile theFile��  ��    O    
    L    	   n        m    ��
�� 
ctnr  o    ���� 0 thefile theFile  m       �                                                                                  MACS  alis    Z  Home                       �<ؿH+    -
Finder.app                                                       ��ǟ1t        ����  	                CoreServices    �=I?      ǟ��      -   �   �  +Home:System:Library:CoreServices:Finder.app    
 F i n d e r . a p p  
  H o m e  &System/Library/CoreServices/Finder.app  / ��        l     ��������  ��  ��        i        I      �� ���� 0 checkexists CheckExists   ��  o      ���� 0 thefile theFile��  ��    O        L    
   I   	�� ��
�� .coredoexbool        obj   o    ���� 0 thefile theFile��    m       �                                                                                  MACS  alis    Z  Home                       �<ؿH+    -
Finder.app                                                       ��ǟ1t        ����  	                CoreServices    �=I?      ǟ��      -   �   �  +Home:System:Library:CoreServices:Finder.app    
 F i n d e r . a p p  
  H o m e  &System/Library/CoreServices/Finder.app  / ��        l     ��������  ��  ��       !   i     " # " I     �� $��
�� .GURLGURLnull��� ��� TEXT $ o      ���� 0 this_url this_URL��   # k     j % %  & ' & l     �� ( )��   ( 0 * display dialog "handling url " & this_URL    ) � * * T   d i s p l a y   d i a l o g   " h a n d l i n g   u r l   "   &   t h i s _ U R L '  + , + l     �� - .��   - 0 * try running with release mode, then debug    . � / / T   t r y   r u n n i n g   w i t h   r e l e a s e   m o d e ,   t h e n   d e b u g ,  0 1 0 l     �� 2 3��   2 9 3 the installed path for the slauncher_helper.app is    3 � 4 4 f   t h e   i n s t a l l e d   p a t h   f o r   t h e   s l a u n c h e r _ h e l p e r . a p p   i s 1  5 6 5 l     �� 7 8��   7 2 ,  base_dir/lib/sirikata/slauncher_helper.app    8 � 9 9 X     b a s e _ d i r / l i b / s i r i k a t a / s l a u n c h e r _ h e l p e r . a p p 6  : ; : l     �� < =��   < , & but we need to run the wrapper binary    = � > > L   b u t   w e   n e e d   t o   r u n   t h e   w r a p p e r   b i n a r y ;  ? @ ? l     �� A B��   A    basedir/bin/slauncher    B � C C .     b a s e d i r / b i n / s l a u n c h e r @  D E D l     �� F G��   F 7 1 so we get a base path, then construct full paths    G � H H b   s o   w e   g e t   a   b a s e   p a t h ,   t h e n   c o n s t r u c t   f u l l   p a t h s E  I J I r      K L K l     M���� M I    �� N��
�� .earsffdralis        afdr N  f     ��  ��  ��   L o      ���� 0 my_path   J  O P O r     Q R Q I    �� S���� 0 getparentpath GetParentPath S  T�� T o   	 
���� 0 my_path  ��  ��   R o      ���� 0 lib_sirikata_path   P  U V U r     W X W I    �� Y���� 0 getparentpath GetParentPath Y  Z�� Z o    ���� 0 lib_sirikata_path  ��  ��   X o      ���� 0 lib_path   V  [ \ [ r    " ] ^ ] I     �� _���� 0 getparentpath GetParentPath _  `�� ` o    ���� 0 lib_path  ��  ��   ^ o      ���� 0 	base_path   \  a b a r   # * c d c l  # ( e���� e b   # ( f g f l  # & h���� h c   # & i j i o   # $���� 0 	base_path   j m   $ %��
�� 
ctxt��  ��   g m   & ' k k � l l  b i n :��  ��   d o      ���� 0 installed_bin_path   b  m n m l  + +�� o p��   o > 8 You need to be careful to get all the settings right to    p � q q p   Y o u   n e e d   t o   b e   c a r e f u l   t o   g e t   a l l   t h e   s e t t i n g s   r i g h t   t o n  r s r l  + +�� t u��   t ? 9 make the shell script run in the background and let this    u � v v r   m a k e   t h e   s h e l l   s c r i p t   r u n   i n   t h e   b a c k g r o u n d   a n d   l e t   t h i s s  w x w l  + +�� y z��   y    script finish up and exit    z � { { 4   s c r i p t   f i n i s h   u p   a n d   e x i t x  | } | r   + . ~  ~ m   + , � � � � � (   >   / d e v / n u l l   2 > & 1   &    o      ���� 0 background_process   }  ��� � Z   / j � ��� � � I   / 7�� ����� 0 checkexists CheckExists �  ��� � b   0 3 � � � o   0 1���� 0 installed_bin_path   � m   1 2 � � � � �  s l a u n c h e r��  ��   � I  : G�� ���
�� .sysoexecTEXT���     TEXT � b   : C � � � b   : A � � � b   : ? � � � l  : = ����� � n   : = � � � 1   ; =��
�� 
psxp � o   : ;���� 0 installed_bin_path  ��  ��   � m   = > � � � � �   s l a u n c h e r   - - u r i = � o   ? @���� 0 this_url this_URL � o   A B���� 0 background_process  ��  ��   � Z   J j � ��� � � I   J R�� ����� 0 checkexists CheckExists �  ��� � b   K N � � � o   K L���� 0 installed_bin_path   � m   L M � � � � �  s l a u n c h e r _ d��  ��   � I  U b�� ���
�� .sysoexecTEXT���     TEXT � b   U ^ � � � b   U \ � � � b   U Z � � � l  U X ����� � n   U X � � � 1   V X��
�� 
psxp � o   U V���� 0 installed_bin_path  ��  ��   � m   X Y � � � � � $ s l a u n c h e r _ d   - - u r i = � o   Z [���� 0 this_url this_URL � o   \ ]���� 0 background_process  ��  ��   � I  e j�� ���
�� .sysodlogaskr        TEXT � m   e f � � � � � > C o u l d n ' t   f i n d   s l a u n c h e r   b i n a r y .��  ��   !  ��� � l     ��������  ��  ��  ��       �� � � � ���   � �������� 0 getparentpath GetParentPath�� 0 checkexists CheckExists
�� .GURLGURLnull��� ��� TEXT � �� ���� � ����� 0 getparentpath GetParentPath�� �� ���  �  ���� 0 thefile theFile��   � ���� 0 thefile theFile �  ��
�� 
ctnr�� � ��,EU � �� ���� � ����� 0 checkexists CheckExists�� �� ���  �  ���� 0 thefile theFile��   � ���� 0 thefile theFile �  ��
�� .coredoexbool        obj �� � �j U � �� #���� � ���
�� .GURLGURLnull��� ��� TEXT�� 0 this_url this_URL��   � ���������������� 0 this_url this_URL�� 0 my_path  �� 0 lib_sirikata_path  �� 0 lib_path  �� 0 	base_path  �� 0 installed_bin_path  �� 0 background_process   � ������ k � ���~ ��} � � ��|
�� .earsffdralis        afdr�� 0 getparentpath GetParentPath
�� 
ctxt� 0 checkexists CheckExists
�~ 
psxp
�} .sysoexecTEXT���     TEXT
�| .sysodlogaskr        TEXT�� k)j  E�O*�k+ E�O*�k+ E�O*�k+ E�O��&�%E�O�E�O*��%k+  ��,�%�%�%j 	Y "*��%k+  ��,�%�%�%j 	Y �j  ascr  ��ޭ