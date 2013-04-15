package com.happhub;

import java.io.IOException;
import java.io.OutputStream;
import java.lang.annotation.Annotation;
import java.lang.reflect.Type;

import javax.ws.rs.Produces;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.MultivaluedMap;
import javax.ws.rs.ext.ContextResolver;
import javax.ws.rs.ext.MessageBodyWriter;
import javax.ws.rs.ext.Provider;
import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Marshaller;

import org.eclipse.persistence.jaxb.MarshallerProperties;

@Provider
@Produces({MediaType.APPLICATION_XML, MediaType.APPLICATION_JSON})
public class JAXBMessageBodyWriter<T> implements MessageBodyWriter<T> {
  @Context
  ContextResolver<JAXBContext> contextResolver;

  @Override
  public boolean isWriteable(Class<?> type, Type genericType, Annotation[] annotations, MediaType mediaType) {
    return true;
  }
 
  @Override
  public long getSize(T object, Class<?> type, Type genericType, Annotation[] annotations, MediaType mediaType) {
    return -1;
  }
 
  @Override
  public void writeTo(T object, Class<?> type, Type genericType, Annotation[] annotations, MediaType mediaType, MultivaluedMap<String, Object> httpHeaders, OutputStream entityStream) throws IOException, WebApplicationException {
    try {
      Marshaller marshaller = contextResolver.getContext(type).createMarshaller();
      marshaller.setProperty(MarshallerProperties.MEDIA_TYPE, mediaType.getType()+"/"+mediaType.getSubtype());
      marshaller.setProperty(MarshallerProperties.JSON_INCLUDE_ROOT, false);
      marshaller.marshal(object, entityStream);
    } catch (JAXBException e) {
      e.printStackTrace();
    }
  }
}