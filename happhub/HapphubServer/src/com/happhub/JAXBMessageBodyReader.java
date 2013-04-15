package com.happhub;

import javax.ws.rs.Consumes;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.MultivaluedMap;
import javax.ws.rs.ext.ContextResolver;
import javax.ws.rs.ext.MessageBodyReader;
import javax.ws.rs.ext.Provider;
import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Unmarshaller;
import javax.xml.transform.stream.StreamSource;

import org.eclipse.persistence.jaxb.UnmarshallerProperties;

@Provider
@Consumes({MediaType.APPLICATION_XML, MediaType.APPLICATION_JSON})
public class JAXBMessageBodyReader<T> implements MessageBodyReader<T> {
  @Context
  ContextResolver<JAXBContext> contextResolver;

  @Override
  public boolean isReadable(java.lang.Class<?> type, java.lang.reflect.Type genericType, java.lang.annotation.Annotation[] annotations, MediaType mediaType) {
    return true;
  }
 
  @Override
  public T readFrom(java.lang.Class<T> type, java.lang.reflect.Type genericType, java.lang.annotation.Annotation[] annotations, MediaType mediaType, MultivaluedMap<java.lang.String,java.lang.String> httpHeaders, java.io.InputStream entityStream) throws java.io.IOException, WebApplicationException {
    try {
      Unmarshaller unmarshaller = contextResolver.getContext(type).createUnmarshaller();
      unmarshaller.setProperty(UnmarshallerProperties.MEDIA_TYPE, mediaType.getType()+"/"+mediaType.getSubtype());
      unmarshaller.setProperty(UnmarshallerProperties.JSON_INCLUDE_ROOT, false);
      return unmarshaller.unmarshal(new StreamSource(entityStream), type).getValue();
    } catch (JAXBException e) {
      e.printStackTrace();
    }
    return null;
  }
}