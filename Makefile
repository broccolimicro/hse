SRCDIR       =  hse
CXXFLAGS	 =  -O2 -g -Wall -fmessage-length=0 -I../ucs -I../petri -I../boolean -I../interpret_boolean -I../interpret_ucs -I../parse_expression -I../parse_ucs -I../parse -I../common
SOURCES	    :=  $(shell find $(SRCDIR) -name '*.cpp')
OBJECTS	    :=  $(SOURCES:%.cpp=%.o)
TARGET		 =  lib$(SRCDIR).a
DEPS = $(subst .o,.d,$(OBJECTS))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rvs $(TARGET) $(OBJECTS)

%.o: $(SRCDIR)/%.cpp 
	$(CXX) $(CXXFLAGS) -c -MMD -MP -o $@ $<
	
-include $(DEPS)

clean:
	rm -f $(OBJECTS) $(TARGET)
