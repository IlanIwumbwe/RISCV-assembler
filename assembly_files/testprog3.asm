li s3, 0xfffffff  # load s3 with 0xff
la s3, 0xfffffff  # load s3 with 0xff  # Godbolt outputs an lui instead of auipc? wth?
mv x2, s3
not x2, x3
neg x2, x3
bgt x2, x3, -2